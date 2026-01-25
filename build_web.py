import os
import re

TEMPLATE = "web/index_template.html"
OUTPUT = "web/index.html"

def main():
    if not os.path.exists(TEMPLATE):
        print(f"Error: Template {TEMPLATE} not found.")
        return

    print(f"Reading template {TEMPLATE}...")
    with open(TEMPLATE, "r", encoding="utf-8") as f:
        content = f.read()

    # Find pattern <!-- INCLUDE: path/to/file.html -->
    matches = re.finditer(r'<!-- INCLUDE: (.*?) -->', content)
    
    # We use a list to store replacements to perform them safely
    replacements = []

    for match in matches:
        marker = match.group(0)
        include_path = match.group(1).strip()
        full_path = os.path.join("web", include_path)
        
        if os.path.exists(full_path):
            print(f"  Including {include_path}")
            with open(full_path, "r", encoding="utf-8") as f:
                include_content = f.read()
                replacements.append((marker, include_content))
        else:
            print(f"  Warning: {include_path} not found")

    # Apply replacements
    for marker, include_content in replacements:
        content = content.replace(marker, include_content)

    print(f"Writing output to {OUTPUT}...")
    with open(OUTPUT, "w", encoding="utf-8") as f:
        f.write(content)
    print("Done.")

if __name__ == "__main__":
    main()
