#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import sys
import shutil

currentPath = os.getcwd()
print("当前工作目录: %s" % currentPath)

buildPath = os.path.join(currentPath, "controller-build-desktop-Qt_4_8_5__qt4_8_5____")
print("构建工作目录: %s" % buildPath)

rmFileName = os.path.join(buildPath, "exconfig.o")
print("删除目标文件: %s" % rmFileName)

# 拷贝到window下的目录
DestPath = "/mnt/hgfs/vmware/controllerRelease_dp/"
# 拷贝到SD卡下的目录
SDPath = "/mnt/hgfs/I/"

targetFile = "controller"

print(" ************************************************************************** ")

def _copyFile(srcFile, destFile):
    if os.path.exists(destFile):
        os.remove(destFile)
        print("清理目标: %s " % destFile)
    
    if os.path.exists(srcFile):
        if os.path.isfile(srcFile):
            shutil.copy2(srcFile, destFile)
            print("拷贝 %s 到 %s 成功" % (srcFile, destFile))
        else:
            print("原文件: %s 不是一个文件" % srcFile)
    else:
        print("原文件: %s 不能存在" % srcFile)


def copy2Win():
    if checkBuildPath(DestPath):
        _copyFile(os.path.join(buildPath, targetFile), os.path.join(DestPath, targetFile))


def copy2SD():
    if os.path.exists(SDPath) and os.path.isdir(SDPath):
        _copyFile(os.path.join(buildPath, targetFile), os.path.join(SDPath, targetFile))

def copyBuildFile():
    copy2Win()
    copy2SD()

def checkBuildPath(path):
    if os.path.exists(path):
        print("检查目录 %s " % path)
    else:
        try:
            os.mkdir(path)
        except FileNotFoundError:
            print("创建目录时出现异常: %s " % FileNotFoundError)
            return False
        else:
            print("新建目录 %s 成功" % path)
    return True


def cleanupFiles(file):
    if os.path.exists(file) and os.path.isfile(file):
        try:
            os.remove(file)
        except OSError:
            print("删除目标文件 %s 失败" % file)
        else:
            print("删除目标文件 %s 成功" % file)
    else:
        print("文件 %s 不存在，无需清理" % file)


def cleanupAll():
    if not os.path.exists(buildPath):
        print("构建目录: %s 不存在，无需清理" % buildPath)
        return

    for file in os.listdir(buildPath):
        filePath = os.path.join(buildPath, file)
        if os.path.isfile(filePath):
            os.remove(filePath)
            print("删除文件: %s" % file)


def printVersionMsg():
    print('''
            build on 2021-8-11 \n
            此脚本用于构建controller项目 \n
            --命令参数rebuild用于重新构建项目 \n
            --命令后无参数或存在无效参数表示一般构建
        ''')

if __name__ == "__main__":
    bBuild = False
    if len(sys.argv) > 1:
        if "rebuild" == sys.argv[1]:
            cleanupAll()
            bBuild = True
        elif "version" == sys.argv[1]:
            printVersionMsg()
    else:
        cleanupFiles(rmFileName)
        bBuild = True
    
    if bBuild:
        print("开始构建项目...")
        if checkBuildPath(buildPath):
            print("切换工作目录: %s " % os.getcwd())
            os.chdir(buildPath)
            print("当前工作目录: %s " % os.getcwd())

            ret = os.system("/opt/qt4.8.5/bin/qmake %s/controller_NuZar/controller.pro -r -spec qws/linux-arm-g++" % currentPath)
            print("qmake return: %d" % ret)
            ret = os.system("make")
            print("make return: %d" % ret)

            if 0 == ret:
                print("项目构建完成...")
                print(" ************************************************************************** ")
                print("拷贝程序文件...")
                copyBuildFile()
                print("拷贝程序文件完成")
            else:
                print("构建失败！")
        else:
            print("项目构建失败...")

