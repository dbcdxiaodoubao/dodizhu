# Multiplayer Dou Dizhu Demo

Windows-native multiplayer Dou Dizhu prototype:

- C++17 game gateway: Boost.Asio, Boost.Beast, Protobuf, TCP and WebSocket.
- Java backend: Java 17, Spring Boot 3, MySQL and Redis.
- Browser client: static HTML, JavaScript, WebSocket.

## Prerequisites

- Docker Desktop running.
- JDK 17 at `C:\Program Files\Microsoft\jdk-17.0.8.7-hotspot`.
- CLion bundled MinGW and CMake, vcpkg at `D:\vcpkg`.

## Start Order

1. Start MySQL and Redis:

```powershell
.\start-infrastructure.ps1
```

2. Build and run the Java backend from IDEA, or run:

```powershell
cd backend
mvn package
cd ..
.\run-backend.ps1
```

3. Build the C++ gateway from CLion using the `windows-mingw-debug` CMake preset, or run:

```powershell
cd cpp
cmake --build --preset windows-mingw-debug-build
cd ..
.\run-gateway.ps1
```

4. Open `http://localhost:8080/` in three browser windows and use different player IDs.

When opening the page from another device on the LAN, use the host machine's IP in the URL. The browser client automatically connects to WebSocket port `9001` on the same host.

The backend accepts `DB_URL`, `DB_USERNAME`, `DB_PASSWORD`, `REDIS_HOST`, `REDIS_PORT`, and `SERVER_PORT` environment overrides. The gateway accepts `BACKEND_HOST` and `BACKEND_PORT`.

## Ports

| Service | Port |
| --- | --- |
| Java backend and web client | 8080 |
| C++ TCP gateway | 9000 |
| C++ WebSocket gateway | 9001 |
| Docker MySQL | 3307 |
| Docker Redis | 6380 |

## Current Features

- Player auto-registration, profile lookup, online Redis cache and game history.
- TCP and WebSocket binary connections using a 6-byte packet header plus Protobuf body.
- Matchmaking, deal, call landlord, play/pass, timeout defaults, settlement, reconnect snapshot and room cleanup.
- Server-authoritative card ownership and base card-pattern validation.
- Browser seat view, hand selection, status snapshots, automatic heartbeat and reconnect.

## Tests

```powershell
cd cpp
ctest --preset windows-mingw-debug-test

cd ..\backend
mvn test
```

## Three-Player Smoke Test

After starting infrastructure, backend, and gateway, install the Python WebSocket client once and run:

```powershell
pip install -r tools\requirements.txt
python tools\smoke_test.py
```

The script opens three WebSocket clients, verifies login, matching, 17-card hand delivery, landlord selection, and the landlord's first play broadcast, then exits.

For a one-command local run that starts and stops backend and gateway automatically:

```powershell
.\run-smoke-test.ps1
```

If the smoke test fails, verify these in order:

1. Docker Desktop is running and `docker compose ps` reports MySQL and Redis as healthy.
2. `mvn package` has produced `backend/target/backend-0.0.1-SNAPSHOT.jar`.
3. The CMake gateway build has produced `cpp/build/gateway.exe`.
4. Python dependencies are installed with `pip install -r tools\requirements.txt`.
