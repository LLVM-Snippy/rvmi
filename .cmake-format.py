# type: ignore
# pylint: skip-file

with section("format"):
    line_width = 100

with section("lint"):
    disabled_codes = [
        "C0111",  # Missing function docstring
        "C0103",  # Invalid name
        "R0915",  # Too many statements
    ]
