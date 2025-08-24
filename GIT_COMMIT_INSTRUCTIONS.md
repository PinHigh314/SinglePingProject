# Git Commit Instructions for SinglePing Project

## Using the Git Commit Script

I've created a batch file `git-commit-project.bat` that makes it easy to commit your project changes to git.

### How to Use:

1. **Run the script:**
   ```bash
   ./git-commit-project.bat
   ```

2. **Enter a commit message** when prompted, or just press Enter for the default message.

3. **The script will:**
   - Check if you're in a git repository
   - Add all changes (including deletions and new files)
   - Commit with your message
   - Show the status after committing

### Manual Git Commands (if preferred):

```bash
# Add all changes
git add --all

# Commit with message
git commit -m "Your commit message here"

# Push to remote repository (if configured)
git push origin main
```

### Current Repository Status:
- **Local branch:** main
- **Remote:** origin/main (GitHub repository)
- **Your branch is ahead by 1 commit** - ready to push

### To Push to GitHub:
```bash
git push origin main
```

The project is already set up with git and connected to the GitHub repository at:
`https://github.com/PinHigh314/SinglePingProject.git`

The git commit script will handle all the file additions and deletions automatically, making it easy to keep your project version controlled.
