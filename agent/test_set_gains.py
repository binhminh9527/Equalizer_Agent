#!/usr/bin/env python3
import argparse
import json
import socket
import sys

DEFAULT_PORT = 5560

def send_gains(gains, host="127.0.0.1", port=DEFAULT_PORT, timeout=3.0):
    if len(gains) != 10:
        raise ValueError("Exactly 10 band gains are required")
    # Build JSON with newline to match simple line-based server
    payload = json.dumps(gains) + "\n"

    with socket.create_connection((host, port), timeout=timeout) as s:
        s.sendall(payload.encode("utf-8"))
        s.shutdown(socket.SHUT_WR)
        resp = s.recv(1024)
        return resp.decode("utf-8", errors="replace").strip()


def parse_args():
    parser = argparse.ArgumentParser(description="Send 10-band gains to Equalizer IPC server")
    parser.add_argument("gains", nargs="*", type=float,
                        help="10 gain values in dB (e.g., 0 3 -2 0 5 0 -3 2 0 1)")
    parser.add_argument("--preset", choices=["flat","bass","treble","vshape"],
                        help="Use a built-in preset instead of manual gains")
    parser.add_argument("--host", default="127.0.0.1", help="IPC server host (default 127.0.0.1)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"IPC server port (default {DEFAULT_PORT})")
    return parser.parse_args()


def get_preset(name):
    presets = {
        "flat":       [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
        "bass":       [4, 3, 2, 1, 0, 0, -1, -2, -3, -4],
        "treble":     [-4, -3, -2, -1, 0, 0, 1, 2, 3, 4],
        "vshape":     [3, 2, 1, 0, -1, -2, 1, 2, 3, 4],
    }
    return presets[name]


def main():
    args = parse_args()

    if args.preset:
        gains = get_preset(args.preset)
    else:
        if len(args.gains) != 10:
            print("Error: provide exactly 10 gains or use --preset", file=sys.stderr)
            sys.exit(2)
        gains = list(args.gains)

    try:
        resp = send_gains(gains, host=args.host, port=args.port)
        print(f"Server response: {resp}")
        print("Sent gains:", json.dumps(gains))
    except Exception as e:
        print(f"Failed to send gains: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
