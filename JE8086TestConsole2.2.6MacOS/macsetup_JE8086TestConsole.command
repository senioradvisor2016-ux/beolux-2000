# Try to remove quarantine from this script first
xattr -d com.apple.quarantine "$0" >/dev/null 2>&1 || true
CURRENT_LOCATION=$(dirname "$0")
cd "$CURRENT_LOCATION"
echo
echo "Clearing extended attributes for JE8086TestConsole"
echo "Location: $CURRENT_LOCATION"
echo
find "$CURRENT_LOCATION" -name 'JE8086TestConsole*' \
    -exec echo "Clearing attributes for: {}" \; \
    -exec xattr -cr {} \; \
     2>&2
echo
echo "Done."
