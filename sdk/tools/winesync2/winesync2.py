#!/usr/bin/env python3
from __future__ import annotations
import sys
import os
import git
import yaml
import shutil

KEEP_FILES = ['CMakeLists.txt', 'precomp.h', 'guid.c']
NOCOPY_FILES = ['Makefile.in']
CONFIG_FILE = '../winesync/winesync.cfg'

class Config:
    def __init__(self, **kwargs) -> None:
        self.reactos = kwargs['reactos']
        self.wine = kwargs['wine']
        if 'wine-staging' in kwargs:
            self.wine_staging = kwargs['wine-staging']
        else:
            self.wine_staging = kwargs['wine_staging']

    @staticmethod
    def load(filename: str) -> Config:
        with open(filename, 'r') as fp:
            yaml_data = yaml.safe_load(fp)
            return Config(**yaml_data['repos'])

    def dump(self, filename: str) -> None:
        with open(filename, 'w') as fp:
            dict2 = {'repos' : self.__dict__ }
            dict2['repos'].update({'wine-staging':dict2['repos']['wine_staging']})
            del dict2['repos']['wine_staging']
            yaml.safe_dump(dict2, fp)

class Module:
    def __init__(self, directories: dict, files: dict, tags: dict) -> None:
        self.directories = directories
        self.files = files
        self.tags = tags

    @staticmethod
    def load(filename: str) -> Module:
        with open(filename, 'r') as fp:
            yaml_data = yaml.safe_load(fp)
            return Module(**yaml_data)

    def dump(self, filename: str) -> None:
        with open(filename, 'w') as fp:
            yaml.safe_dump(self.__dict__, fp)

class WineSync:
    def __init__(self, wine_version: str) -> None:
        if not os.path.exists(CONFIG_FILE):
            self.make_config(CONFIG_FILE)
        else:
            self.cfg = Config.load(CONFIG_FILE)

        self.setup_wine_repos(wine_version)
        self.wine_version = wine_version

    def sync_module(self, module_name: str) -> None:
        module = Module.load('../winesync/%s.cfg' % module_name)
        for wine_dir, ros_dir in module.directories.items():  # directory parse
            # delete files
            for root, dirs, files in os.walk('%s/%s' % (self.cfg.reactos, ros_dir)):
                files[:] = [file for file in files if '/' not in file and '\\' not in file]

                for file in files:
                    if file not in KEEP_FILES:
                        os.remove('%s/%s' % (root, file))

            # start file copy
            for root, dirs, files in os.walk('%s/%s' % (self.cfg.wine, wine_dir), topdown=True):
                files[:] = [file for file in files if '/' not in file and '\\' not in file]

                for file in files:
                    if file not in NOCOPY_FILES:
                        shutil.copy2('%s/%s' % (root, file), '%s/%s/%s' % (self.cfg.reactos, ros_dir, file))

            dir_name = wine_dir[wine_dir.rindex('/') + 1:]
            if os.path.exists('patches/%s.diff' % dir_name):
                print('Applying patch %s.diff...' % dir_name)
                os.system('patch -d \"%s/%s\" -i patches/%s.diff' % (self.cfg.reactos, dir_name, dir_name))
        for wine_file, ros_file in module.files.items():  # file parse
            shutil.copy2('%s/%s' % (self.cfg.wine, wine_file), '%s/%s' % (self.cfg.reactos, ros_file))

        # update wine tags version
        module.tags['wine'] = 'wine-%s' % self.wine_version
        module.dump('../winesync/%s.cfg' % module_name)


    def setup_wine_repos(self, version: str) -> None:
        repo = git.Repo(self.cfg.wine)
        wine_tag = 'wine-' + version
        if not wine_tag in repo.tags:
            raise Exception('Invalid wine version ' + version)

        tag = repo.tags[wine_tag]

        new_head = repo.create_head('winesync2-branch-wine-' + version, tag.commit)
        repo.git.reset('--hard', 'winesync2-branch-wine-' + version)
        repo.head.reference = new_head

        repo = git.Repo(self.cfg.wine_staging)
        wine_tag = "v" + version
        if not wine_tag in repo.tags:
            raise Exception('Invalid wine staging version ' + version)

        tag = repo.tags[wine_tag]
        new_head = repo.create_head('winesync2-branch-wine-' + version, tag.commit)
        repo.git.reset('--hard', 'winesync2-branch-wine-' + version)
        repo.head.reference = new_head

    def make_config(self, config_file: str) -> None:
        reactos_dir = input('ReactOS directory: ')
        wine_dir = input('Wine directory: ')
        wine_staging_dir = input('Wine staging directory: ')
        self.cfg = Config(reactos= reactos_dir, wine= wine_dir, wine_staging= wine_staging_dir)
        self.cfg.dump(config_file)

if len(sys.argv) < 3:
    print('Usage: %s [wine-version] [module1] [module2] [...]' % sys.argv[0])
else:
    ws = WineSync(sys.argv[1])
    for i in range(2, len(sys.argv)):
        ws.sync_module(sys.argv[i])
