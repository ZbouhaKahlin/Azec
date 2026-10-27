# Security policy

## Supported versions

Security fixes target the latest stable release on
[GitHub Releases](https://github.com/Azec-Zuckerberg/Azec/releases). Older
releases and experimental branches do not receive separate security updates.

## Report a vulnerability privately

Use [Report a vulnerability](https://github.com/Azec-Zuckerberg/Azec/security/advisories/new)
in this repository's Security tab. Do not open a public issue containing an
exploit, pairing credentials, a token link, personal profiles, or private logs.

Include the affected version, Windows version, relevant settings, reproduction
steps using disposable data, and the expected impact. A minimal demonstration
is more useful than access to a real installation. Remove credentials from
attachments. Please allow time to investigate and coordinate a fix before
publishing details; there is no guaranteed response time or bug bounty.

## Relevant boundaries

The optional phone interface is a local-network HTTP service protected by
pairing and a session token. HTTP does not encrypt traffic. Use a trusted LAN;
do not expose the service through router forwarding or public hosting. Treat
the code, revealed QR, and token link as credentials. New code disconnects
previous sessions, and phone control can be disabled in Settings.

Update downloads use HTTPS and are checked against the release asset's SHA-256
before execution. This detects changed downloads; it does not protect against
a compromised maintainer account or release infrastructure. Interception setup
is explicit, and local DLL loading checks the pinned supported file. Azec does
not attest the installed kernel driver or make anti-cheat guarantees.

Review or reproduce issues with isolated dry-run fixtures as described in
[Testing](docs/testing.md), without sending mouse output or touching live data.
