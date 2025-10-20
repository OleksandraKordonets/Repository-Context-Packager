# Repository-Context-Packager
OSD600 - Release 0.1

This is a command-line tool that analyzes local git repositories and creates a text file containing repository content optimized for sharing with Large Language Models (LLMs).

When developers want to get help from ChatGPT or other LLMs about their code, they often struggle with how to share their codebase effectively. They might copy-paste individual files, but this loses important context about the project structure, dependencies, and relationships between files. This tool will solve this by automatically collecting and formatting repository content into a single, well-structured text file that can be easily shared with any LLM.

---

## Features:

**Output to File**  
```
./repository-context-packager . -o output.txt
./repository-context-packager . --output context-package.md
```
**File Filtering by Extension**  
```
# Only include JavaScript files
./repository-context-packager . --include "*.js"

# Include multiple extensions
./repository-context-packager . --include "*.js,*.py,*.md"
```
**Recent Files Mode**
```
./repository-context-packager . --recent
```
**Directory-Only Mode**
```
./repository-context-packager . --dirs-only
```
**Exclude by Pattern**
```
# Exclude all test JS files
./repository-context-packager . --exclude-pattern "test.*\.js$"
```
**Compress file's contents**
```
# Display only functions' signatures and comments
./repository-context-packager . --compress
```

---

## Installation:

### Prerequisites:
- **g++** (C++17 or later)
- **Git** (to clone the repository)

### Clone the repository:
```
git clone https://github.com/OleksandraKordonets/Repository-Context-Packager.git
cd Repository-Context-Packager
```

### Build:
From inside the repo folder, run:
```
g++ src/main.cpp src/cli.cpp src/FileReader.cpp src/GitInfoCollector.cpp src/OutputFormatter.cpp src/RepositoryScanner.cpp src/Compressor.cpp -o repository-context-packager.exe
```
**Note:** Using `*.cpp` instead of listing all the files may not work reliably across platforms, so it’s recommended to use the full command above.

### Run:
Once built, you can run the tool from the command line:
```
./repository-context-packager --help
```

### Usage:
```
# Package the current directory
./repository-context-packager .

# Package a specific repo directory
./repository-context-packager /home/student1/Seneca/major-project

# Package specific files
./repository-context-packager src/main.js src/utils.h

# Package with output file
./repository-context-packager . -o context.txt

# Package only JavaScript files
./repository-context-packager . --include "*.cpp"

# Only package files modified in the last 7 days
tool-name . --recent

# Can be combined with other flags
tool-name . -r --output recent-changes.txt

# Show only directories
./repository-context-packager . --dirs-only

# Exclude files by regex
./repository-context-packager . --exclude-pattern ".*\.md$"

# Compress files
./repository-context-packager . --compress
```

---

## Output Format:
````markdown
# Repository Context

## File System Location

/absolute/path/to/repo/being/analyzed

## Git Info

- Commit: <commit-sha>
- Branch: <branch-name>
- Author: <author-name>
- Date: <commit-date>

## Structure
```
src/
  main.js
  utils/
    helper.js
package.json
```

## File Contents

### File: package.json
```json
{
  "name": "my-project",
  "version": "1.0.0"
}
```

### File: src/main.js
```javascript
const helper = require('./utils/helper');

function main() {
  console.log('Hello World');
}
```

### File: src/utils/helper.js
```javascript
function formatString(str) {
  return str.trim();
}

module.exports = { formatString };
```

## Summary
- Total files: 3
- Total lines: 14
````

---

##  License

[MIT License © 2025 OleksandraKordonets](https://github.com/OleksandraKordonets/Repository-Context-Packager/blob/main/LICENSE)
