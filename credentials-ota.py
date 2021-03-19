Import ("env")

import re

print("Finding and exporting OTA password...")

pattern = re.compile("#define OTA_PASSWORD\s*" + '"' + "(.*)" + '"')

for i, line in enumerate(open('include\credentials.h')):
    for match in re.finditer(pattern, line):
        print('Found OTA_PASSWORD on line %s: %s' % (i+1, match.group(1)))
        break

env["OTA_PASSWORD"] = match.group(1)
