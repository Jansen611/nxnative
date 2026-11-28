import plistlib
from pathlib import Path, PurePosixPath
import sys
import zipfile


def VerifyPrewarmDisabled(path):
    path = Path(path)
    if path.suffix.lower() == '.ipa':
        with zipfile.ZipFile(path) as archive:
            candidates = [name for name in archive.namelist()
                          if len(PurePosixPath(name).parts) == 3
                          and PurePosixPath(name).parts[0] == 'Payload'
                          and PurePosixPath(name).parts[1].endswith('.app')
                          and PurePosixPath(name).parts[2] == 'Info.plist']
            if len(candidates) != 1:
                raise ValueError('expected exactly one main app Info.plist')
            info = plistlib.loads(archive.read(candidates[0]))
    else:
        info = plistlib.loads(path.read_bytes())
    if not isinstance(info, dict):
        raise ValueError('Info.plist must contain a dictionary')
    if 'DMPrewarmUploadURL' in info:
        raise ValueError('Info.plist must not contain DMPrewarmUploadURL')


if __name__ == '__main__':
    try:
        if len(sys.argv) != 2:
            raise ValueError('expected a plist or IPA path')
        VerifyPrewarmDisabled(sys.argv[1])
    except (OSError, ValueError, zipfile.BadZipFile, plistlib.InvalidFileException) as error:
        sys.exit(f'[RED] prewarm upload policy: {error}')