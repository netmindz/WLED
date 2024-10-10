Import('env')
import json
import os

PACKAGE_FILE = "package.json"

if(os.environ.get("WLED_VERSION")):
    version = os.environ.get("WLED_VERSION")
else:
    with open(PACKAGE_FILE, "r") as package:
        version = json.load(package)["version"]

env.Append(BUILD_FLAGS=[f"-DWLED_VERSION={version}"])
