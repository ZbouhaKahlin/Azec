#include "core.hpp"
#include <string>
int main(int argc,char** argv) {
    if(argc==3&&std::string(argv[1])=="--defaults")return r6::Config{}.save(argv[2])?0:1;
    if(argc==4&&std::string(argv[1])=="--roundtrip") {
        r6::Config config;return config.load(argv[2])&&config.save(argv[3])?0:1;
    }
    return r6::self_test(argc>1?argv[1]:"test-config.bin")?0:1;
}
