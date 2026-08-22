file(READ "${INPUT}" text)

string(REGEX REPLACE "(^|\n)([ \t]*)#[ \t]+([0-9]+)" "\\1\\2#line \\3" text "${text}")

file(WRITE "${OUTPUT}" "${text}")
