#!/bin/bash

cd "$(dirname "$0")"

case "${1:-}" in
    clean)
        rm -f *.obj *.ps *.sfc *.sym linkfile main.asm gfx.asm 2>/dev/null || true
        echo "Cleaned."
        exit 0
        ;;
    debug)
        export PVSNESLIB_DEBUG=1
        ;;
esac

# Clean intermediates (preserve hdr.asm)
rm -f *.obj *.ps linkfile main.asm gfx.asm 2>/dev/null || true

# Build (filter noise, ignore sed error on macOS)
make 2>&1 | grep -v "debug mode is\|compilation is enabled\|unterminated substitute\|sed:\|make: \*\*\*"

# Check result
if [[ -f starshmup.sfc ]]; then
    echo ""
    echo "ROM ready: starshmup.sfc ($(stat -f%z starshmup.sfc) bytes)"
else
    echo "Build failed!"
    exit 1
fi
