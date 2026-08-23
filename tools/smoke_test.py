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


async def receive_until(ws, message_id: int) -> bytes:
    while True:
        packet = await asyncio.wait_for(ws.recv(), timeout=5)
        _, received_id = struct.unpack(">IH", packet[:6])
        if received_id == message_id:
            return packet


def read_varint(data: bytes, index: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while index < len(data):
        byte = data[index]
        index += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, index
        shift += 7
    raise ValueError("invalid varint")


def deal_cards(packet: bytes) -> list[int]:
    cards: list[int] = []
    index = 6
    while index < len(packet):
        tag, index = read_varint(packet, index)
        field = tag >> 3
        wire_type = tag & 7
        if field == 1 and wire_type == 2:
            length, index = read_varint(packet, index)
            end = index + length
            while index < end:
                card, index = read_varint(packet, index)
                cards.append(card)
        elif wire_type == 0:
            _, index = read_varint(packet, index)
        else:
            break
    return cards


async def wait_all(clients, message_id: int) -> list[bytes]:
    return await asyncio.gather(*(receive_until(client, message_id) for client in clients))


async def main() -> None:
    suffix = uuid.uuid4().hex[:8]
    player_ids = [f"smoke-{name}-{suffix}" for name in ("a", "b", "c")]
    async with websockets.connect(WS_URL) as first, \
            websockets.connect(WS_URL) as second, \
            websockets.connect(WS_URL) as third:
        clients = [first, second, third]

        for client, player_id in zip(clients, player_ids):
            await client.send(frame(1, login_body(player_id)))
            await receive_until(client, 2)

        for client in clients:
            await client.send(frame(5))
        await wait_all(clients, 6)
        deals = await wait_all(clients, 13)
        hands = [deal_cards(deal) for deal in deals]
        if any(len(hand) != 17 for hand in hands):
            raise RuntimeError("each player must receive exactly 17 cards")

        for seat, call_landlord in enumerate((True, False, False)):
            await clients[seat].send(frame(7, b"\x08\x01" if call_landlord else b"\x08\x00"))
            await wait_all(clients, 8)

        await first.send(frame(9, b"\x08" + bytes([hands[0][0]])))
        await wait_all(clients, 17)
        print("Three-player login, match, deal, call-landlord, and first-play smoke test passed.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as error:
        print(f"Smoke test failed: {error}", file=sys.stderr)
        raise SystemExit(1)
