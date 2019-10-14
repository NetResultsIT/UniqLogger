import os
import sys
import platform
import re
import fileinput
import zipfile

version = "x.y.z"
hash = "abcdef"


def get_git_hash(folder):
    # print("Calling git command: " + 'git --git-dir=' + folder + '/.git log -1 --pretty=format:"%h"')
    h = os.popen('git --git-dir=' + folder + '/.git log -1 --pretty=format:"%h"').read()
    print("Found git hash in repo: " + h)
    if h is None:
        h = "0"
    return h


def get_platform_name():
    if os.environ.get('UNQL_OS'):
        return os.environ['UNQL_OS']
    p = sys.platform
    if p == 'darwin':
        p = 'macos'
    return p


def test_platform():
    def linux_distribution():
        try:
            return platform.linux_distribution()
        except:
            return "N/A"

    print("""Python version: %s
    dist: %s
    linux_distribution: %s
    system: %s
    machine: %s
    platform: %s
    uname: %s
    version: %s
    mac_ver: %s
    """ % (
        sys.version.split('\n'),
        str(platform.dist()),
        linux_distribution(),
        platform.system(),
        platform.machine(),
        platform.platform(),
        platform.uname(),
        platform.version(),
        platform.mac_ver(),
    ))


def get_pkg_name():
    platform = get_platform_name()
    if platform == 'windows':
        ext = "zip"
    else:
        ext = "tgz"
    name = "uniqlogger-" + version + "-" + platform + "-r" + hash + "." + ext
    return name


def parse_version(in_filename):
    for line in fileinput.FileInput(in_filename):
        if re.search("VERSION", line):
            sl = line.split()
            pversion = sl[2]
            return pversion
    return version


def zipdir(path, ziph):
    # ziph is zipfile handle
    abs_src = os.path.abspath(path)
    for root, dirs, files in os.walk(path):
        for file in files:
            #ziph.write(os.path.join(root, file))
            absname = os.path.abspath(os.path.join(root, file))
            arcname = "uniqlogger/" + absname[len(abs_src) + 1:]
            print 'zipping %s as %s' % (os.path.join(root, file),
                                        arcname)
            ziph.write(absname, arcname)


def create_pkg(in_pkgname):
    if in_pkgname.endswith("zip"):
        # create zip file
        zipf = zipfile.ZipFile(in_pkgname, 'w', zipfile.ZIP_DEFLATED)
        zipdir('last_build/', zipf)
        zipf.close()
    else:
        os.system("ln -sf last_build uniqlogger")
        os.system("tar -cvzhf " + get_pkg_name() + " uniqlogger/")
        os.system("rm uniqlogger")


if __name__ == '__main__':
    if len(sys.argv) > 1:
        print("Error - this script does not need params")
        sys.exit(1)
    hash = get_git_hash(os.path.abspath('..'))
    # print(get_platform_name())
    version = parse_version("uniqlogger.pro")
    create_pkg(get_pkg_name())

