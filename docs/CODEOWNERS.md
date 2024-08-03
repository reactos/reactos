# About `CODEOWNERS`

The purpose of the `CODEOWNERS` file is to give newcomers to the project the responsible developers when submitting a pull request on GitHub, or opening a bug report on Jira.

`CODEOWNERS` will notably establish who is responsible for a specific area of ReactOS. Being a maintainer means the following:
- The person has good knowledge in the area
- The person is able to enforce consistency in the area
- The person may be available for giving help in the area
- The person has push access on the repository

Being a maintainer does not mean the following:
- The person is dedicated to the area
- The person is working full-time on the area/on ReactOS
- The person is paid
- The person is always available

**We have no supported (paid) areas in ReactOS.**

When submitting a pull request on GitHub and looking for reviewers, look at `CODEOWNERS` and ask for a review from some of the people listed in the matching area. Also, assign the pull request to the assigned maintainer. Don't ask for a review from all the listed reviewers. Also, when submitting a pull request on GitHub, rules defined in [CONTRIBUTING.md](CONTRIBUTING.md) apply. If the maintainer is not available and reviewers approved the pull request, developers feeling confident can merge the pull request. Note that reviewers do not necessarily have push access to the repository. When submitting a bug report on Jira, if you want to be sure to have a developer with skills in that area, ping the assigned maintainer.

There should be one and only one primary maintainer per area.

In case of 3rd party code (also referred as upstream), the maintainer is responsible for updating the source code and managing local patches. He or she is not here to upstream code on your behalf. As responsible, he may refuse a local patch if you did not try to upstream your changes.

If you want to get listed in `CODEOWNERS`, open a pull request.

This file uses GitHub's format for specifying code owners.
- Lines starting with `#` are comment lines.
- All other lines specify a path / file (wildcards allowed) followed by the GitHub user name(s) of the code owners. See https://help.github.com/en/articles/about-code-owners for more information.


## See Also

- [CODEOWNERS](CODEOWNERS)