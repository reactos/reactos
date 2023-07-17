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
- Make sure the author has specified a real e-mail in all PR commits
- If PR contains code or translations, make sure the author has not specified a nickname or alias, but a full legal name in all PR commits
- If PR contains media (wallpapers, themes, icons, sounds) or out-of-code documentation, make sure the author has specified the name or alias in all PR commits
- If PR contains mixed code with media changes, handle it as PR with code
- Important notes before using "Squash and merge" strategy on a PR:
  - Make sure the author's name in GitHub profile matches one in commits. If this is not the case, ask the author to set it accordingly.
  - If the author does not want to set the name in GitHub profile:
    - "no squash merge" label needs to be added to a PR.
	- Make sure every commit message is formatted correctly as in [.gitmessage](https://github.com/reactos/reactos/blob/master/.gitmessage).
	- Finally in this case a PR has to be merged either using "Rebase and merge" strategy or manually.
  - By pressing "Squash and merge" button in a PR you can make sure the author does not use no-reply e-mail -
  under the commit message there will be a text label saying: `This commit will be authored by <address@email.com>`
