import os
import sys
import argparse

def main():
    parser = argparse.ArgumentParser(description="Convert Apache's mime.types file to our MimeTypes.inc")
    parser.add_argument('-s', '--source', required=True, type=str, help='Path to source mime.types file')
    parser.add_argument('-o', '--output', required=True, type=str, help='Path to target MimeTypes.inc file to overwrite')

    args = parser.parse_args()
    if not os.path.isfile(args.source):
        print("Source mime.types file must exist")
        parser.print_help()
        return -1

    mapping = {}

    with open(args.source, "r") as fh:
        for line in fh.readlines():
            if line.startswith("#"):
                continue
            line = line.strip()
            parts = line.split()
            mimetype = parts[0].strip()
            exts = parts[1:]
            if not exts:
                print(f"No extensions for {mimetype}, skipping")
                continue
            
            for ext in exts:
                mapping.update({ext: mimetype})

    with open(args.output, "w") as fh:
        for ext, mimetype in mapping.items():
            fh.write(f'STR_PAIR("{ext}", "{mimetype}"),\n')

    return 0

if __name__ == '__main__':
    sys.exit(main())
