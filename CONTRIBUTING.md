# Contributing Guidelines
There are several ways to contribute to ReactOS. You can [make a donation](https://reactos.org/donate/), [file a bug report](https://jira.reactos.org/), [add documentation to our wiki](https://reactos.org/wiki) or submit a code contribution. This document focuses on our guidelines for submitting code contributions.

> [!CAUTION]
> If you have worked with private Windows source code, or contributed to projects derived from such code, we cannot accept contributions from you in any area related to that work. **Leaked Windows source code and the Windows Research Kernel (WRK) are considered private.**
>
> Publicly available information, including documentation, headers, and patents, may be used for research purposes.

> [!WARNING]
> In order to comply with international copyright law, we require all code contributions to be made using your legal identity. Using your legal identity allows contributions to be audited against individuals known to have access to Windows source code.

## Rules
- **Use your legal name and a real email.** We do not accept anonymous code contributions. Every commit must have the author's full legal name. We recommend having the same name and email set on your GitHub profile.

> [!NOTE]
> Media, including wallpapers, themes, icons, and sounds, can be contributed using an alias.

- **Respect our [Coding Style](https://reactos.org/wiki/Coding_Style) and [Programming Guidelines](https://reactos.org/wiki/Programming_Guidelines).**
- **Keep your contribution small and focused.** Large pull requests take a long time to review since maintainers must consider all the implications of your changes. It's more effective to contribute several smaller pull requests.
- **AI-assisted contributions must be well understood.** You may use AI to assist your work, but you must be able to explain how your code works. Contributions that appear to be fully AI-generated will be rejected.
- **Do not be afraid to ask questions.** Ask our developers in our [official chat](https://chat.reactos.org/).

## Making a Code Contribution
We use [Git](https://git-scm.com/) as our version control system and [GitHub](https://github.com/reactos/reactos) to manage code contributions. Code contributions on GitHub are called [pull requests](https://docs.github.com/en/pull-requests/reference/pull-requests), which consist of one or more [Git commits](https://docs.github.com/en/pull-requests/reference/commits). Project maintainers review pull requests and merge approved pull requests into the source tree. For more information about how we manage pull requests, see [Rules for Managing Pull Requests](PULL_REQUEST_MANAGEMENT.md).

Commit messages must be prefixed with the components updated in square brackets. Include any related Jira tickets in the message. A commit message template is available in [.gitmessage](.gitmessage).

## What Can I Work On?
- **Fix tests.** Test results can be found at [reactos.org/testman](https://reactos.org/testman). Generally speaking, tests that pass on Windows but fail on ReactOS are considered ReactOS defects.
- **Fix bugs.** Bugs are tracked on [Jira](https://jira.reactos.org/). Bugs that are expected to be easy to solve have the `starter-project` label. You can find a list of starter projects [here](https://jira.reactos.org/issues/?jql=labels%20%3D%20starter-project).

> [!IMPORTANT]
> Contributions for third party code, such as Wine, should be sent upstream.

- **Add new features.** ReactOS is not feature complete. Like bugs, feature requests are also tracked on Jira. You can find a list of feature requests [here](https://jira.reactos.org/issues/?jql=project%20%3D%20CORE%20AND%20issuetype%20%3D%20%22New%20Feature%22%20AND%20status%20%3D%20Open).

> [!WARNING]
> Many missing features are protected by international patents. Do not contribute features currently protected by patents.

- **Fix Coverity scans.** [Coverity](https://scan.coverity.com/projects/reactos) is a static code analysis tool that can detect hard to find defects. You can request access [here](https://scan.coverity.com/memberships/new?project_id=reactos).

# See Also
- [Rules for managing Pull Requests](PULL_REQUEST_MANAGEMENT.md)
