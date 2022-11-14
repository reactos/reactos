# Rules for managing Pull Requests

For the sake of trying to maintain an acceptable number of open but idle PRs, the following rules should be considered:
- If a PR has at least one approval, it can be merged after 1 week of waiting for additional comments.
  - If the change has at least 3 approvals or you consider it trivial enough, it may be merged right away.
- If a PR stays in "changes requested" for too long, and there is no indication from the author that they are working on it, it shall be closed.
  - Rule of thumb: 2 weeks for a small PR. Can be longer if the PR is large.
  - The PR can be reopened at any point, if you have additional comments, or new changes have been done.
- If you require a review from a particular person, assign the PR to that person. Don't just rely on the "review requested" feature of GitHub.
- Remember that PR labels exist. You can assign an appropriate label to a pull request to designate it's scope, grab additional attention or just for extra navigation possibilities.
- Don't feel obliged to comment everything you see, just for the sake of commenting. Be it on JIRA, GitHub, or even on IRC.

In addition, in order to avoid coming off as rude to helpful contributors, please refrain from:
- Asking the contributor to do unrelated work
- Closing without providing a reason
- Merging with the intention to rewrite that code soon after

Before merging a PR, make sure it follows the [contributing rules](CONTRIBUTING.md#rules-and-recommendations), but more importantly:
- Make sure the author does not use a nickname or alias, but a full legal name instead, and have specified a real e-mail in all PR commits
- Make sure the author does have full name set in GitHub profile that matches one in commits
- By pressing "Squash and merge" button in a PR you can make sure the author does not use no-reply e-mail -
  under the commit message there will be a text label saying: `This commit will be authored by <address@email.com>`