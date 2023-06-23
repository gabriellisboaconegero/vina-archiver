#! /usr/bin/python3
import sys

vals = [234, 139, 83, 356, 98]
chars = "#$%&*"
ptr = 0
i = 0
for char, r in zip(chars, vals):
    print(f"-{char * r}-", end="")
    print(f"[{char}] -> off_set: {ptr}, size: {r + 2}", file=sys.stderr)
    ptr += r + 2

print(f"total size: {ptr}", file=sys.stderr)
