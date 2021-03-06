/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryabstractdeploystep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QTimer>
#include <QEventLoop>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryAbstractDeployStep::BlackBerryAbstractDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id)
    : ProjectExplorer::BuildStep(bsl, id)
    , m_processCounter(-1)
    , m_process(0)
    , m_timer(0)
    , m_futureInterface(0)
    , m_eventLoop(0)
{
    connect(&m_outputParser, SIGNAL(addTask(ProjectExplorer::Task)), this, SIGNAL(addTask(ProjectExplorer::Task)));
    connect(&m_outputParser, SIGNAL(progressParsed(int)), this, SLOT(reportProgress(int)));
}

BlackBerryAbstractDeployStep::BlackBerryAbstractDeployStep(ProjectExplorer::BuildStepList *bsl, BlackBerryAbstractDeployStep *bs)
    : ProjectExplorer::BuildStep(bsl, bs)
    , m_processCounter(-1)
    , m_process(0)
    , m_timer(0)
    , m_futureInterface(0)
    , m_eventLoop(0)
{
    connect(&m_outputParser, SIGNAL(addTask(ProjectExplorer::Task)), this, SIGNAL(addTask(ProjectExplorer::Task)));
    connect(&m_outputParser, SIGNAL(progressParsed(int)), this, SLOT(reportProgress(int)));
}

BlackBerryAbstractDeployStep::~BlackBerryAbstractDeployStep()
{
    delete m_process;
    m_process = 0;
}

bool BlackBerryAbstractDeployStep::init()
{
    m_params.clear();
    m_processCounter = -1;

    m_environment = target()->activeBuildConfiguration()->environment();
    m_buildDirectory = target()->activeBuildConfiguration()->buildDirectory().toString();
    // The bar descriptor is using BUILD_DIR variable to set the target path
    m_environment.appendOrSet(QLatin1String("BUILD_DIR"), m_buildDirectory);

    return true;
}

void BlackBerryAbstractDeployStep::run(QFutureInterface<bool> &fi)
{
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop;

    fi.setProgressRange(0, 100 * m_params.size());

    Q_ASSERT(!m_futureInterface);
    m_futureInterface = &fi;

    runCommands();

    bool returnValue = m_eventLoop->exec();

    // Finished
    m_params.clear();
    m_processCounter = -1;

    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    delete m_process;
    m_process = 0;
    delete m_eventLoop;
    m_eventLoop = 0;

    m_futureInterface = 0;

    fi.reportResult(returnValue);
}

void BlackBerryAbstractDeployStep::addCommand(const QString &command, const QStringList &arguments)
{
    ProjectExplorer::ProcessParameters param;
    param.setCommand(command);
    param.setArguments(arguments.join(QLatin1Char(' ')));
    m_params << param;
}

void BlackBerryAbstractDeployStep::reportProgress(int progress)
{
    QTC_ASSERT(progress >= 0 && progress <= 100, return);

    if (m_futureInterface)
        m_futureInterface->setProgressValue(100 * m_processCounter + progress);
}

void BlackBerryAbstractDeployStep::runCommands()
{
    if (!m_process) {
        m_process = new Utils::QtcProcess();
        connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyReadStdOutput()), Qt::DirectConnection);
        connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processReadyReadStdError()), Qt::DirectConnection);
    }

    m_process->setEnvironment(m_environment);
    m_process->setWorkingDirectory(m_buildDirectory);

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(handleProcessFinished(int,QProcess::ExitStatus)), Qt::DirectConnection);

    runNextCommand();
}

void BlackBerryAbstractDeployStep::runNextCommand()
{
    ++m_processCounter;
    m_futureInterface->setProgressValue(100 * m_processCounter);

    ProjectExplorer::ProcessParameters param = m_params.takeFirst();

    QTC_ASSERT(m_process->state() == QProcess::NotRunning, return);

    m_process->setCommand(param.effectiveCommand(), param.effectiveArguments());
    m_process->start();
    if (!m_process->waitForStarted()) {
        m_eventLoop->exit(false);
        return;
    }
    processStarted(param);
}

void BlackBerryAbstractDeployStep::processStarted(const ProjectExplorer::ProcessParameters &params)
{
    emitOutputInfo(params, params.prettyArguments());
}

void BlackBerryAbstractDeployStep::emitOutputInfo(const ProjectExplorer::ProcessParameters &params, const QString &arguments)
{
    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(params.effectiveCommand()),
                        arguments),
                   BuildStep::MessageOutput);
}

void BlackBerryAbstractDeployStep::raiseError(const QString &errorMessage)
{
    ProjectExplorer::Task task = ProjectExplorer::Task(ProjectExplorer::Task::Error, errorMessage, Utils::FileName(), -1,
                                                       ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    emit addTask(task, 1);
    emit addOutput(errorMessage, BuildStep::ErrorMessageOutput);
}

void BlackBerryAbstractDeployStep::processReadyReadStdOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

void BlackBerryAbstractDeployStep::stdOutput(const QString &line)
{
    m_outputParser.stdOutput(line);
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
}

void BlackBerryAbstractDeployStep::processReadyReadStdError()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        const QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

void BlackBerryAbstractDeployStep::checkForCancel()
{
    if (m_futureInterface->isCanceled()
         && m_timer && m_timer->isActive()) {
        m_timer->stop();
        if (m_process) {
            m_process->terminate();
            m_process->waitForFinished(5000); //while waiting, the process can be killed
            if (m_process)
                m_process->kill();
        }
        if (m_eventLoop)
            m_eventLoop->exit(false);
    }
}

void BlackBerryAbstractDeployStep::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        if (!m_params.isEmpty())
            runNextCommand();
        else
            m_eventLoop->exit(true);
    } else {
        m_eventLoop->exit(false);
    }
}

void BlackBerryAbstractDeployStep::stdError(const QString &line)
{
    m_outputParser.stdError(line);
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
}
