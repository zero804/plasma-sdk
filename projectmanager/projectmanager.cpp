/*
  Copyright (c) 2009 Lim Yuen Hoe <yuenhoe@hotmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QDir>

#include <KMessageBox>
#include <KLocalizedString>
#include <KZip>
#include <KUrl>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "projectmanager.h"
#include "startpage.h"

ProjectManager::ProjectManager(QWidget* parent) : QDialog(parent)
{
    projectList = new QListWidget();
    projectList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(projectList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(emitProjectSelected()));

    loadButton = new QPushButton(i18n("Load Project"));
    connect(loadButton, SIGNAL(clicked()), this, SLOT(emitProjectSelected()));

    deleteButton = new QPushButton(i18n("Delete Project"));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(confirmDeletion()));

    QHBoxLayout *hoz = new QHBoxLayout();
    hoz->addWidget(loadButton);
    hoz->addWidget(deleteButton);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->addWidget(projectList);
    lay->addLayout(hoz);
    setLayout(lay);
}

void ProjectManager::confirmDeletion()
{
    //TODO: might want to disallow deleting a currently active project, or handle it
    //      gracefully somehow.
    QString dialogText = i18n("Are you sure you want to delete the selected projects? This is not undoable!");
    int code = KMessageBox::warningContinueCancel(this, dialogText);
    if (code != KMessageBox::Continue) return;
    QList<QListWidgetItem*> l = projectList->selectedItems();

    //TODO: should probably centralize config handling code somewhere.
    KConfigGroup cg = KGlobal::config()->group("General");
    QStringList recentFiles = cg.readEntry("recentFiles", QStringList());
    for (int i=0; i < l.size(); i++) {
        QString folder = l[i]->data(StartPage::FullPathRole).value<QString>();
        QString path = KStandardDirs::locateLocal("appdata", folder + '/');
        deleteProject(path);
        if (recentFiles.contains(folder)) {
            recentFiles.removeAt(recentFiles.indexOf(folder));
        }
    }
    //TODO: should perform a success check here instead of assuming success.
    cg.writeEntry("recentFiles", recentFiles);
    KGlobal::config()->sync();
    emit requestRefresh();
}

void ProjectManager::addProject(QListWidgetItem *item)
{
    projectList->addItem(item);
}

void ProjectManager::clearProjects()
{
    projectList->clear();
}

void ProjectManager::emitProjectSelected()
{
    QList<QListWidgetItem*> l = projectList->selectedItems();
    if (l.size() != 1) {
      KMessageBox::information(this, i18n("Please select exactly one project to load."));
      return;
    }
    QString url = l[0]->data(StartPage::FullPathRole).value<QString>();

    emit projectSelected(url, QString());
    done(QDialog::Accepted);
}

bool ProjectManager::exportPackage(const KUrl &toExport, const KUrl &targetFile)
{
    bool ret = true;
    KZip plasmoid(targetFile.path());
    if (!plasmoid.open(QIODevice::WriteOnly)) {
        return false;
    }
    ret = plasmoid.addLocalDirectory(toExport.path(), ".");
    plasmoid.close();
    return ret;
}

bool ProjectManager::importPackage(const KUrl &toImport, const KUrl &targetLocation)
{
    bool ret = true;
    KZip plasmoid(toImport.path());
    if (!plasmoid.open(QIODevice::ReadOnly)) {
        return false;
    }
    plasmoid.directory()->copyTo(targetLocation.path());
    plasmoid.close();
    return ret;
}

bool ProjectManager::deleteProject(const KUrl &projectLocation)
{
    removeDirectory(projectLocation.path());
    return true;
}

void ProjectManager::removeDirectory(const QString& path)
{
    QDir d(path);
    QFileInfoList list = d.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
    for (int i=0; i<list.size(); i++) {
        if (list[i].isDir() && list[i].filePath() != path) {
            removeDirectory(list[i].filePath());
        }
        d.remove(list[i].fileName());
    }
    d.rmpath(path);
}