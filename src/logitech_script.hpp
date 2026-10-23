#pragma once
#include "core.hpp"
#include "version.hpp"
#include <QtCore>
#include <algorithm>

// Logitech G HUB output backend.
//
// G HUB runs the generated Lua inside the mouse driver, so the game receives
// real device movement instead of a Windows SendInput injection (SendInput
// events carry the "injected" flag and the current Rainbow Six anti-cheat drops
// them once the game has focus). Azec stays the editor and the profile store:
// it writes Azec.lua and the user assigns that script to Rainbow Six in G HUB.
//
// Nothing here validates against a real installation; detection only decides
// where the file is written. Writing is atomic through QSaveFile.
namespace logitech {
inline QString dataDirectory(){
    const auto local=qEnvironmentVariable("LOCALAPPDATA");
    return local.isEmpty()?QString():QDir(local).filePath("LGHUB");
}

// G HUB keeps its data next to the user profile and its binaries in Program
// Files; either one means the app is present.
inline bool installed(){
    const auto local=qEnvironmentVariable("LOCALAPPDATA");
    const auto programFiles=qEnvironmentVariable("PROGRAMFILES");
    const auto programFilesX86=qEnvironmentVariable("PROGRAMFILES(X86)");
    const QStringList candidates{
        dataDirectory(),
        local.isEmpty()?QString():QDir(local).filePath("Programs/LGHUB/lghub.exe"),
        programFiles.isEmpty()?QString():QDir(programFiles).filePath("LGHUB/lghub.exe"),
        programFilesX86.isEmpty()?QString():QDir(programFilesX86).filePath("LGHUB/lghub.exe")};
    for(const auto& path:candidates)if(!path.isEmpty()&&QFileInfo::exists(path))return true;
    return false;
}

// The scripts folder is where G HUB lists user scripts; without G HUB the file
// lands next to the profiles so the user can still copy it later.
inline QString scriptPath(){
    const auto hub=dataDirectory();
    if(installed()&&!hub.isEmpty()&&QDir().mkpath(hub+"/scripts"))
        return QDir(hub+"/scripts").filePath("Azec.lua");
    return QDir::current().filePath("azec_logitech.lua");
}

namespace detail {
inline QString number(double value){return QString::number(value,'f',2);}
inline QString string(const QString& value){
    QString escaped=value;escaped.replace('\\',"\\\\");escaped.replace('"',"\\\"");
    return QLatin1Char('"')+escaped+QLatin1Char('"');
}
inline QString entry(const r6::Operator& op){
    return QStringLiteral("{ v = %1, h = %2, rapid = %3, humanize = %4 }")
        .arg(number(op.vertical),number(op.horizontal),
             op.rapid?QStringLiteral("true"):QStringLiteral("false"),
             op.humanize?QStringLiteral("true"):QStringLiteral("false"));
}
}

inline QByteArray script(const r6::Config& config,bool adsOnly){
    using namespace detail;
    QString out;
    out+="-- Azec " AZEC_VERSION " · Logitech G HUB recoil script\n";
    out+="-- Generated from your saved Azec profiles. Export again after editing them.\n";
    out+="--\n";
    out+="-- 1. Open G HUB, select (or create) the profile used for Rainbow Six Siege.\n";
    out+="-- 2. Open Scripting for that profile and load this file, or paste it in.\n";
    out+="-- 3. Keep the profile assigned to RainbowSix.exe: the script only runs there.\n";
    out+="--\n";
    out+="-- Controls: hold right + left = recoil control\n";
    out+="--           mouse 4 = next operator, mouse 5 = primary/secondary\n";
    out+="--           middle click = toggle rapid fire\n";
    out+="-- Tuning: SETTINGS.strength scales every pull; G HUB moves the mouse\n";
    out+="-- through the driver, so it can differ from the old SendInput output.\n\n";

    out+="local SETTINGS = {\n";
    out+="  strength = 1.00,       -- multiplies every vertical and horizontal pull\n";
    out+="  adsRequired = ";
    out+=adsOnly?QStringLiteral("true"):QStringLiteral("false");
    out+=",     -- only compensate while aiming\n";
    out+="  recoilInterval = 12,   -- ms between pulls (Azec uses 12)\n";
    out+="  rapidInterval = 30,    -- ms between shots when rapid fire is on\n";
    out+="  rapidClickHold = 25,   -- ms the simulated trigger stays down\n";
    out+="  startDelay = 0,        -- ms before the first pull\n";
    out+="  verbose = true,        -- log the active profile in the G HUB console\n";
    out+="}\n\n";

    out+="local operators = {\n";
    for(int id=0;id<r6::operator_count;++id){
        const auto& primary=config.operators[id];
        out+="  { name = "+string(QString::fromUtf8(primary.name))+", primary = "+entry(primary);
        if(config.secondary_enabled[id])out+=", secondary = "+entry(config.secondary[id]);
        out+=" },\n";
    }
    out+="}\n\n";

    out+="local startIndex = "+QString::number(std::clamp(config.selected,0,r6::operator_count-1)+1)+"\n";
    out+="local slot = 1\n";
    out+="local running = false\n";
    out+="local rapidOverride = nil\n\n";

    out+="EnablePrimaryMouseButtonEvents(true)\n\n";
    out+="local function note(text)\n";
    out+="  if SETTINGS.verbose then OutputLogMessage(\"[Azec] \" .. text .. \"\\n\") end\n";
    out+="end\n\n";
    out+="local function profile()\n";
    out+="  local operator = operators[startIndex]\n";
    out+="  local data = (slot == 2) and operator.secondary or operator.primary\n";
    out+="  if not data then slot = 1; data = operator.primary end\n";
    out+="  return operator, data\n";
    out+="end\n\n";
    out+="local function rapidActive(data)\n";
    out+="  if rapidOverride ~= nil then return rapidOverride end\n";
    out+="  return data.rapid == true\n";
    out+="end\n\n";
    out+="local function describe()\n";
    out+="  local operator, data = profile()\n";
    out+="  note(string.format(\"%s | %s | V %.1f H %.1f | rapid %s\",operator.name,\n";
    out+="    (slot == 2) and \"secondary\" or \"primary\",data.v,data.h,tostring(rapidActive(data))))\n";
    out+="end\n\n";
    out+="-- C++ truncation: Azec accumulates fractional movement and drops the rest.\n";
    out+="local function truncate(value)\n";
    out+="  if value >= 0 then return math.floor(value) end\n";
    out+="  return math.ceil(value)\n";
    out+="end\n\n";
    out+="local function fire()\n";
    out+="  if running then return end\n";
    out+="  local _, data = profile()\n";
    out+="  local rapid = rapidActive(data)\n";
    out+="  local x, y = 0.0, 0.0\n";
    out+="  local started = false\n";
    out+="  running = true\n";
    out+="  repeat\n";
    out+="    if IsMouseButtonPressed(1) and (not SETTINGS.adsRequired or IsMouseButtonPressed(3)) then\n";
    out+="      if not started then\n";
    out+="        started = true\n";
    out+="        if SETTINGS.startDelay > 0 then Sleep(SETTINGS.startDelay) end\n";
    out+="      end\n";
    out+="      local jitter = 0\n";
    out+="      if data.humanize then jitter = math.random(-1, 1) end\n";
    out+="      x = x + data.h * SETTINGS.strength + jitter\n";
    out+="      y = y + data.v * SETTINGS.strength + jitter\n";
    out+="      local mx, my = truncate(x), truncate(y)\n";
    out+="      if mx ~= 0 or my ~= 0 then MoveMouseRelative(mx, my) end\n";
    out+="      x = x - mx\n";
    out+="      y = y - my\n";
    out+="      if rapid then\n";
    out+="        ReleaseMouseButton(1)\n";
    out+="        Sleep(5)\n";
    out+="        PressMouseButton(1)\n";
    out+="        Sleep(SETTINGS.rapidClickHold)\n";
    out+="        ReleaseMouseButton(1)\n";
    out+="      end\n";
    out+="      Sleep(rapid and SETTINGS.rapidInterval or SETTINGS.recoilInterval)\n";
    out+="    else\n";
    out+="      started = false\n";
    out+="      x, y = 0.0, 0.0\n";
    out+="      Sleep(8)\n";
    out+="    end\n";
    out+="  until not IsMouseButtonPressed(1)\n";
    out+="  -- Hand the trigger back to the physical button instead of leaving ours down.\n";
    out+="  if rapid then ReleaseMouseButton(1) end\n";
    out+="  running = false\n";
    out+="end\n\n";
    out+="function OnEvent(event, arg)\n";
    out+="  if event ~= \"MOUSE_BUTTON_PRESSED\" then return end\n";
    out+="  if arg == 1 then\n";
    out+="    if IsMouseButtonPressed(3) or not SETTINGS.adsRequired then fire() end\n";
    out+="  elseif arg == 4 then\n";
    out+="    startIndex = startIndex % #operators + 1\n";
    out+="    slot = 1\n";
    out+="    describe()\n";
    out+="  elseif arg == 5 then\n";
    out+="    if operators[startIndex].secondary then slot = (slot == 2) and 1 or 2 end\n";
    out+="    describe()\n";
    out+="  elseif arg == 3 then\n";
    out+="    local _, data = profile()\n";
    out+="    rapidOverride = not rapidActive(data)\n";
    out+="    describe()\n";
    out+="  end\n";
    out+="end\n";
    return out.toUtf8();
}

inline QByteArray read(const QString& path){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))return {};
    return file.readAll();
}

inline bool write(const QByteArray& bytes,const QString& path,QString* path_out=nullptr,QString* error=nullptr){
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()){
        if(error)*error=file.errorString();
        file.cancelWriting();
        return false;
    }
    if(!file.commit()){if(error)*error=file.errorString();return false;}
    if(path_out)*path_out=path;
    return true;
}
} // namespace logitech
