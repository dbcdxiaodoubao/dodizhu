"""Manual three-player WebSocket smoke test for a running backend and gateway."""

import asyncio
import struct
import sys
import uuid

import websockets

WS_URL = "ws://127.0.0.1:9001"


def frame(message_id: int, body: bytes = b"") -> bytes:
    return struct.pack(">IH", 6 + len(body), message_id) + body


def login_body(player_id: str) -> bytes:
    encoded = player_id.encode("utf-8")
    return b"\x0a" + bytes([len(encoded)]) + encoded


async def receive_until(ws, message_ids: set[int]) -> dict[int, bytes]:
    received: dict[int, bytes] = {}
    while not message_ids.issubset(received):
        packet = await asyncio.wait_for(ws.recv(), timeout=5)
        _, received_id = struct.unpack(">IH", packet[:6])
        received.setdefault(received_id, packet)
    return received


async def player(player_id: str, call_landlord: bool) -> None:
    async with websockets.connect(WS_URL) as ws:
        await ws.send(frame(1, login_body(player_id)))
        await receive_until(ws, {2})
        await ws.send(frame(5))
        matched = await receive_until(ws, {6, 13})
        deal = matched[13]
        print(f"{player_id}: received {len(deal) - 6} bytes of hand data")
        await ws.send(frame(7, b"\x08\x01" if call_landlord else b"\x08\x00"))
        await receive_until(ws, {8})


async def main() -> None:
    suffix = uuid.uuid4().hex[:8]
    await asyncio.gather(
        player(f"smoke-a-{suffix}", True),
        player(f"smoke-b-{suffix}", False),
        player(f"smoke-c-{suffix}", False),
    )
    print("Three-player login, match, deal, and call-landlord smoke test passed.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as error:
        print(f"Smoke test failed: {error}", file=sys.stderr)
        raise SystemExit(1)
