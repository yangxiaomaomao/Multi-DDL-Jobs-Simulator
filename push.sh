#!/bin/bash

# Check if commit message is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <commit-message>"
    exit 1
fi

# Add all changes to staging
git add .

# Commit with the provided message
git commit -m "$1"

# Push changes to the remote repository
git push origin main