from __future__ import annotations

import asyncio
import json

import httpx
import websockets


HTTP_URL = "http://127.0.0.1:8770"
WS_URL = "ws://127.0.0.1:8770/ws/agent/manual-check"


async def main() -> None:
    async with httpx.AsyncClient(timeout=5) as client:
        response = await client.get(f"{HTTP_URL}/health")
        response.raise_for_status()
        print("HTTP /health:")
        print(json.dumps(response.json(), ensure_ascii=False, indent=2))

    async with websockets.connect(WS_URL) as socket:
        connected = json.loads(await socket.recv())
        print("\nWebSocket connected:")
        print(json.dumps(connected, ensure_ascii=False, indent=2))
        await socket.send(json.dumps({"type": "ping"}))
        pong = json.loads(await socket.recv())
        print("\nWebSocket ping:")
        print(json.dumps(pong, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    asyncio.run(main())
