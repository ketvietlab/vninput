#!/usr/bin/env python3
"""Generate the native Input menu's 16-point vector V badge, without dependencies.

This is a template symbol, separate from the full-size application logo.
The PDF MediaBox fixes its logical size, including on Retina displays.
"""
from pathlib import Path

content = b"""0 G
1.4 w
1 J 1 j
4 1 m 12 1 l 13.657 1 15 2.343 15 4 c
15 12 l 15 13.657 13.657 15 12 15 c
4 15 l 2.343 15 1 13.657 1 12 c
1 4 l 1 2.343 2.343 1 4 1 c h S
1.8 w
4.8 11.2 m 8 4.8 l 11.2 11.2 l S
"""
objects = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 16 16] /Resources << >> /Contents 4 0 R >>",
    f"<< /Length {len(content)} >>\nstream\n".encode() + content + b"endstream",
]
pdf = bytearray(b"%PDF-1.4\n")
offsets = [0]
for number, obj in enumerate(objects, 1):
    offsets.append(len(pdf))
    pdf.extend(f"{number} 0 obj\n".encode() + obj + b"\nendobj\n")
xref = len(pdf)
pdf.extend(f"xref\n0 {len(offsets)}\n0000000000 65535 f \n".encode())
for offset in offsets[1:]:
    pdf.extend(f"{offset:010d} 00000 n \n".encode())
pdf.extend(f"trailer\n<< /Size {len(offsets)} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n".encode())
destination = Path(__file__).resolve().parents[1] / "resources/branding/vninput-input-source.pdf"
destination.write_bytes(pdf)
print(destination)
