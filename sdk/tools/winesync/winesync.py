#!/usr/bin/env python3

import sys
import os
import posixpath
import string
import argparse
import subprocess
import fnmatch
import pygit2
import yaml

def string_to_valid_file_name(to_convert):
    valid_chars = '-_.()' + string.ascii_letters + string.digits
    result = ''
    for c in to_convert:
        if c in valid_chars:
            result += c
        else:
            result += '_'
    # strip final dot, if any
    if result.endswith('.'):
        return result[:-1]
    return result

class wine_sync:
    def __init__(self, module):
        if os.path.isfile('winesync.cfg'):
            with open('winesync.cfg', 'r') as file_input:
                config = yaml.safe_load(file_input)
            self.reactos_src = config['repos']['reactos']
            self.wine_src = config['repos']['wine']
            self.wine_staging_src = config['repos']['wine-staging']
        else:
            config = { }
            self.reactos_src = input('Please enter the path to the reactos git tree: ')
            self.wine_src = input('Please enter the path to the wine git tree: ')
            self.wine_staging_src = input('Please enter the path to the wine-staging git tree: ')
            config['repos'] = { 'reactos': self.reactos_src,
                                'wine': self.wine_src,
                                'wine-staging': self.wine_staging_src }
            with open('winesync.cfg', 'w') as file_output:
                yaml.dump(config, file_output)

        self.wine_repo = pygit2.Repository(self.wine_src)
        self.wine_staging_repo = pygit2.Repository(self.wine_staging_src)
        self.reactos_repo = pygit2.Repository(self.reactos_src)

        # the standard author signature we will use
        self.winesync_author_signature = pygit2.Signature('winesync', 'ros-dev@reactos.org')

        # read the index from the reactos tree
        self.reactos_index = self.reactos_repo.index
        self.reactos_index.read()

        # get the actual state for the asked module
        self.module = module
        with open(module + '.cfg', 'r') as file_input:
            self.module_cfg = yaml.safe_load(file_input)

        self.staged_patch_dir = posixpath.join('sdk', 'tools', 'winesync', self.module + '_staging')

    def create_or_checkout_wine_branch(self, wine_tag, wine_staging_tag):
        # build the wine branch name
        wine_branch_name = 'winesync-' + wine_tag
        if wine_staging_tag:
            wine_branch_name += '-' + wine_staging_tag

        branch = self.wine_repo.lookup_branch(wine_branch_name)
        if branch is None:
            # get our target commits
            wine_target_commit = self.wine_repo.revparse_single(wine_tag)
            if isinstance(wine_target_commit, pygit2.Tag):
                wine_target_commit = wine_target_commit.target
            if isinstance(wine_target_commit, pygit2.Commit):
                wine_target_commit = wine_target_commit.id

            # do the same for the wine-staging tree
            if wine_staging_tag:
                wine_staging_target_commit = self.wine_staging_repo.revparse_single(wine_staging_tag)
                if isinstance(wine_staging_target_commit, pygit2.Tag):
                    wine_staging_target_commit = wine_staging_target_commit.target
                if isinstance(wine_staging_target_commit, pygit2.Commit):
                    wine_staging_target_commit = wine_staging_target_commit.id

            self.wine_repo.branches.local.create(wine_branch_name, self.wine_repo.revparse_single('HEAD'))
            self.wine_repo.checkout(self.wine_repo.lookup_branch(wine_branch_name))
            self.wine_repo.reset(wine_target_commit, pygit2.GIT_RESET_HARD)

            # do the same for the wine-staging tree
            if wine_staging_tag:
                self.wine_staging_repo.branches.local.create(wine_branch_name, self.wine_staging_repo.revparse_single('HEAD'))
                self.wine_staging_repo.checkout(self.wine_staging_repo.lookup_branch(wine_branch_name))
                self.wine_staging_repo.reset(wine_staging_target_commit, pygit2.GIT_RESET_HARD)

                # run the wine-staging script
                if subprocess.call(['python', self.wine_staging_src + '/staging/patchinstall.py', 'DESTDIR=' + self.wine_src, '--all', '--backend=git-am']):
                    # the new script failed (it doesn't exist?), try the old one
                    subprocess.call(['bash', '-c', self.wine_staging_src + '/patches/patchinstall.sh DESTDIR=' + self.wine_src + ' --all --backend=git-am'])

                # delete the branch we created
                self.wine_staging_repo.checkout(self.wine_staging_repo.lookup_branch('master'))
                self.wine_staging_repo.branches.delete(wine_branch_name)
        else:
            self.wine_repo.checkout(self.wine_repo.lookup_branch(wine_branch_name))

        return wine_branch_name

    # Helper function for resolving wine tree path to reactos one
    # Note: it doesn't care about the fact that the file actually exists or not
    def wine_to_reactos_path(self, wine_path):
        if self.module_cfg['files'] and (wine_path in self.module_cfg['files']):
            # we have a direct mapping
            return self.module_cfg['files'][wine_path]

        if not '/' in wine_path:
            # root files should have a direct mapping
            return None

        wine_dir, wine_file = os.path.split(wine_path)
        if self.module_cfg['directories'] and (wine_dir in self.module_cfg['directories']):
            # we have a mapping for the directory
            return posixpath.join(self.module_cfg['directories'][wine_dir], wine_file)

        # no match
        return None

    def sync_wine_commit(self, wine_commit, in_staging, staging_patch_index):
        # Get the diff object
        diff = self.wine_repo.diff(wine_commit.parents[0], wine_commit)

        modified_files = False
        ignored_files = []
        warning_message = ''
        complete_patch = ''

        if in_staging:
            # see if we already applied this patch
            patch_file_name = f'{staging_patch_index:04}-{string_to_valid_file_name(wine_commit.message.splitlines()[0])}.diff'
            patch_dir = os.path.join(self.reactos_src, self.staged_patch_dir)
            patch_path = os.path.join(patch_dir, patch_file_name)
            if os.path.isfile(patch_path):
                print(f'Skipping patch as {patch_path} already exists')
                return True, ''

        for delta in diff.deltas:
            if delta.status == pygit2.GIT_DELTA_ADDED:
                # check if we should care
                new_reactos_path = self.wine_to_reactos_path(delta.new_file.path)
                if not new_reactos_path is None:
                    warning_message += 'file ' + delta.new_file.path + ' is added to the wine tree!\n'
                    old_reactos_path = '/dev/null'
                else:
                    old_reactos_path = None
            elif delta.status == pygit2.GIT_DELTA_DELETED:
                # check if we should care
                old_reactos_path = self.wine_to_reactos_path(delta.old_file.path)
                if not old_reactos_path is None:
                    warning_message += 'file ' + delta.old_file.path + ' is removed from the wine tree!\n'
                    new_reactos_path = '/dev/null'
                else:
                    new_reactos_path = None
            elif delta.new_file.path.endswith('Makefile.in'):
                warning_message += 'file ' + delta.new_file.path + ' was modified!\n'
                # no need to warn that those are ignored, we just did.
                continue
            else:
                new_reactos_path = self.wine_to_reactos_path(delta.new_file.path)
                old_reactos_path = self.wine_to_reactos_path(delta.old_file.path)

            if (new_reactos_path is not None) or (old_reactos_path is not None):
                # print('Must apply diff: ' + old_reactos_path + ' --> ' + new_reactos_path)
                if delta.status == pygit2.GIT_DELTA_ADDED:
                    new_blob = self.wine_repo.get(delta.new_file.id)
                    blob_patch = pygit2.Patch.create_from(
                        old=None,
                        new=new_blob,
                        new_as_path=new_reactos_path)
                elif delta.status == pygit2.GIT_DELTA_DELETED:
                    old_blob = self.wine_repo.get(delta.old_file.id)
                    blob_patch = pygit2.Patch.create_from(
                        old=old_blob,
                        new=None,
                        old_as_path=old_reactos_path)
                else:
                    new_blob = self.wine_repo.get(delta.new_file.id)
                    old_blob = self.wine_repo.get(delta.old_file.id)

                    blob_patch = pygit2.Patch.create_from(
                        old=old_blob,
                        new=new_blob,
                        old_as_path=old_reactos_path,
                        new_as_path=new_reactos_path)

                # print(str(wine_commit.id))
                # print(blob_patch.text)

                # this doesn't work
                # reactos_diff = pygit2.Diff.parse_diff(blob_patch.text)
                # reactos_repo.apply(reactos_diff)
                try:
                    subprocess.run(['git', '-C', self.reactos_src, 'apply', '--reject'], input=blob_patch.data, check=True)
                except subprocess.CalledProcessError as err:
                    warning_message += 'Error while applying patch to ' + new_reactos_path + '\n'

                if delta.status == pygit2.GIT_DELTA_DELETED:
                    try:
                        self.reactos_index.remove(old_reactos_path)
                    except IOError as err:
                        warning_message += 'Error while removing file ' + old_reactos_path + '\n'
                # here we check if the file exists. We don't complain, because applying the patch already failed anyway
                elif os.path.isfile(os.path.join(self.reactos_src, new_reactos_path)):
                    self.reactos_index.add(new_reactos_path)

                complete_patch += blob_patch.text

                modified_files = True
            else:
                ignored_files += [delta.old_file.path, delta.new_file.path]

        if not modified_files:
            # We applied nothing
            return False, ''

        print('Applied patches from wine commit ' + str(wine_commit.id))

        if ignored_files:
            warning_message += 'WARNING: some files were ignored: ' + ' '.join(ignored_files) + '\n'

        if not in_staging:
            self.module_cfg['tags']['wine'] = str(wine_commit.id)
            with open(self.module + '.cfg', 'w') as file_output:
                yaml.dump(self.module_cfg, file_output)
            self.reactos_index.add(f'sdk/tools/winesync/{self.module}.cfg')
        else:
            # Add the staging patch
            # do not save the wine commit ID in <module>.cfg, as it's a local one for staging patches
            if not os.path.isdir(patch_dir):
                os.mkdir(patch_dir)
            with open(patch_path, 'w') as file_output:
                file_output.write(complete_patch)
            self.reactos_index.add(posixpath.join(self.staged_patch_dir, patch_file_name))

        self.reactos_index.write()

        commit_msg = f'[WINESYNC] {wine_commit.message}\n'
        if (in_staging):
            commit_msg += f'wine-staging patch by {wine_commit.author.name} <{wine_commit.author.email}>'
        else:
            commit_msg += f'wine commit id {str(wine_commit.id)} by {wine_commit.author.name} <{wine_commit.author.email}>'

        self.reactos_repo.create_commit(
            'HEAD',
            self.winesync_author_signature,
            self.reactos_repo.default_signature,
            commit_msg,
            self.reactos_index.write_tree(),
            [self.reactos_repo.head.target])

        if (warning_message != ''):
            warning_message += 'If needed, amend the current commit in your reactos tree and start this script again'

            if not in_staging:
                warning_message += f'\n' \
                    f'You can see the details of the wine commit here:\n' \
                    f'    https://source.winehq.org/git/wine.git/commit/{str(wine_commit.id)}\n'
            else:
                patch_file_path = posixpath.join(self.staged_patch_dir, patch_file_name)
                warning_message += f'\n' \
                    f'Do not forget to run\n' \
                    f'    git diff HEAD^ \':(exclude){patch_file_path}\' > {patch_file_path}\n' \
                    f'after your correction and then\n' \
                    f'    git add {patch_file_path}\n' \
                    f'before running "git commit --amend"'

        return True, warning_message

    def revert_staged_patchset(self):
        # revert all of this in one commit
        staged_patch_dir_path = posixpath.join(self.reactos_src, self.staged_patch_dir)
        if not os.path.isdir(staged_patch_dir_path):
            return True

        has_patches = False

        for patch_file_name in sorted(os.listdir(staged_patch_dir_path), reverse=True):
            patch_path = os.path.join(staged_patch_dir_path, patch_file_name)
            if not os.path.isfile(patch_path):
                continue

            has_patches = True

            with open(patch_path, 'rb') as patch_file:
                try:
                    subprocess.run(['git', '-C', self.reactos_src, 'apply', '-R', '--ignore-whitespace', '--reject'], stdin=patch_file, check=True)
                except subprocess.CalledProcessError as err:
                    print(f'Error while reverting patch {patch_file_name}')
                    print('Please check, remove the offending patch with git rm, and relaunch this script')
                    return False

            self.reactos_index.remove(posixpath.join(self.staged_patch_dir, patch_file_name))
            self.reactos_index.write()
            os.remove(patch_path)

        if not has_patches:
            return True

        # Note: these path lists may be empty or None, in which case
        # we should not call index.add_all(), otherwise we would add
        # any untracked file present in the repository.
        if self.module_cfg['files']:
            self.reactos_index.add_all([f for f in self.module_cfg['files'].values()])
        if self.module_cfg['directories']:
            self.reactos_index.add_all([f'{d}/*.*' for d in self.module_cfg['directories'].values()])
        self.reactos_index.write()

        self.reactos_repo.create_commit(
            'HEAD',
            self.winesync_author_signature,
            self.reactos_repo.default_signature,
            f'[WINESYNC]: revert wine-staging patchset for {self.module}',
            self.reactos_index.write_tree(),
            [self.reactos_repo.head.target])
        return True

    def sync_to_wine(self, wine_tag, wine_staging_tag):
        # Get our target commit
        wine_target_commit = self.wine_repo.revparse_single(wine_tag)
        if isinstance(wine_target_commit, pygit2.Tag):
            wine_target_commit = wine_target_commit.target
        if isinstance(wine_target_commit, pygit2.Commit):
            wine_target_commit = wine_target_commit.id
        # print(f'wine target commit is {wine_target_commit}')

        # get the wine commit id where we left
        in_staging = False
        wine_last_sync = self.wine_repo.revparse_single(self.module_cfg['tags']['wine'])
        if isinstance(wine_last_sync, pygit2.Tag):
            if not self.revert_staged_patchset():
                return
            wine_last_sync = wine_last_sync.target
        if isinstance(wine_last_sync, pygit2.Commit):
            wine_last_sync = wine_last_sync.id

        # create a branch to keep things clean
        wine_branch_name = self.create_or_checkout_wine_branch(wine_tag, wine_staging_tag)

        finished_sync = True
        staging_patch_index = 1

        # walk each commit between last sync and the asked tag/revision
        wine_commit_walker = self.wine_repo.walk(self.wine_repo.head.target, pygit2.GIT_SORT_TOPOLOGICAL | pygit2.GIT_SORT_REVERSE)
        wine_commit_walker.hide(wine_last_sync)
        for wine_commit in wine_commit_walker:
            applied_patch, warning_message = self.sync_wine_commit(wine_commit, in_staging, staging_patch_index)

            if str(wine_commit.id) == str(wine_target_commit):
                print('We are now in staging territory')
                in_staging = True

            if not applied_patch:
                continue

            if in_staging:
                staging_patch_index += 1

            if warning_message != '':
                print("THERE WERE SOME ISSUES WHEN APPLYING THE PATCH\n\n")
                print(warning_message)
                print("\n")
                finished_sync = False
                break

        # we're done without error
        if finished_sync:
            # update wine tag and commit
            self.module_cfg['tags']['wine'] = wine_tag
            with open(self.module + '.cfg', 'w') as file_output:
                yaml.dump(self.module_cfg, file_output)

            self.reactos_index.add(f'sdk/tools/winesync/{self.module}.cfg')
            self.reactos_index.write()
            self.reactos_repo.create_commit(
                'HEAD',
                self.winesync_author_signature,
                self.reactos_repo.default_signature,
                f'[WINESYNC]: {self.module} is now in sync with wine-staging {wine_tag}',
                self.reactos_index.write_tree(),
                [self.reactos_repo.head.target])

            print('The branch ' + wine_branch_name + ' was created in your wine repository. You might want to delete it, but you should keep it in case you want to sync more module up to this wine version')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('module', help='The module you want to sync. <module>.cfg must exist in the current directory.')
    parser.add_argument('wine_tag', help='The wine tag or commit id to sync to.')
    parser.add_argument('wine_staging_tag', nargs='?', default=None, help='The optional wine staging tag or commit id to pick wine staged patches from.')

    args = parser.parse_args()

    syncator = wine_sync(args.module)

    return syncator.sync_to_wine(args.wine_tag, args.wine_staging_tag)


if __name__ == '__main__':
    main()
