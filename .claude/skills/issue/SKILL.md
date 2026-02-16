# Issue Handling Skill

When working on a GitHub issue:

1. **Fetch issue details**: Use `gh issue view <number>` to get the full issue description
2. **Create a branch**: `git checkout -b issue-<number>` from `main`
3. **Plan**: Enter plan mode to explore the codebase and design the implementation
4. **Implement**: Make the changes
5. **Add tests**: Write tests for the feature or bug fix, including config compatibility tests for any new settings
6. **Test**: Run `make test` to verify all tests pass
7. **Build**: Run `make mac_dev` to build for local manual verification
8. **Commit**: Create a commit referencing the issue (e.g., "Fix #<number>: description")
9. **PR**: Create a pull request referencing the issue with `gh pr create`
