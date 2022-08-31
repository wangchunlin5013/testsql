/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     wangchunlin<wangchunlin@uniontech.com>
 *
 * Maintainer: wangchunlin<wangchunlin@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "widget.h"

#include <QDebug>
#include <QTableView>
#include <QToolButton>
#include <QString>
#include <QSqlQuery>

static const char kConnectionName[] = "MYCONNECTION";
static const char kDatabaseName[] = "MYDATABASE";

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // select
    QToolButton *selectSign = new QToolButton(this);
    selectSign->setText(tr("sign"));

    QToolButton *selectRegister = new QToolButton(this);
    selectRegister->setText(tr("register"));

    QHBoxLayout *selectLayout = new QHBoxLayout();
    selectLayout->addWidget(selectSign);
    selectLayout->addWidget(selectRegister);

    // sign
    QFrame *signFrame = new QFrame(this);

    QLabel *signNameLabel = new QLabel(signFrame);
    signNameLabel->setText(tr("user name:"));

    QLineEdit *signNameLineEdit = new QLineEdit(signFrame);
    signNameLineEdit->setPlaceholderText(tr("input user name"));

    QLabel *signPwdLabel = new QLabel(signFrame);
    signPwdLabel->setText(tr("user password:"));

    QLineEdit *signPwdLineEdit = new QLineEdit(signFrame);
    signPwdLineEdit->setPlaceholderText(tr("input password"));

    QGridLayout *signLayout = new QGridLayout();
    signLayout->addWidget(signNameLabel, 0, 0);
    signLayout->addWidget(signNameLineEdit, 0, 1);
    signLayout->addWidget(signPwdLabel, 1, 0);
    signLayout->addWidget(signPwdLineEdit, 1, 1);

    QPushButton *signBtn = new QPushButton(signFrame);
    signBtn->setText(tr("sign in"));
    QPushButton *passwordBtn = new QPushButton(signFrame);
    passwordBtn->setText("forget password");
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(signBtn);
    btnLayout->addWidget(passwordBtn);

    signBtn->setEnabled(false);
    signPwdLineEdit->setEchoMode(QLineEdit::Password);

    QVBoxLayout *signMainLayout = new QVBoxLayout();
    signMainLayout->addLayout(signLayout);
    signMainLayout->addLayout(btnLayout);

    signFrame->setLayout(signMainLayout);

    // register
    QFrame *registerFrame = new QFrame(this);

    QLabel *todoLabel = new QLabel(registerFrame);
    todoLabel->setText(tr("todo"));

    QVBoxLayout *registerMainLayout = new QVBoxLayout();
    registerMainLayout->addWidget(todoLabel);

    registerFrame->setLayout(registerMainLayout);

    // main layout
    QStackedLayout *stackLayout = new QStackedLayout();
    stackLayout->addWidget(signFrame);
    stackLayout->addWidget(registerFrame);
    stackLayout->setCurrentIndex(0);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(selectLayout);
    mainLayout->addLayout(stackLayout);

    this->setLayout(mainLayout);

    // sign in logic
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        qWarning() << "warining: unalbe to load database,can not find sqlite";
        return;
    }

    if (!initDb())
        return;

    connect(selectSign, &QToolButton::clicked, this, [=] () {
        if (stackLayout->currentIndex() != 0) {
            stackLayout->setCurrentIndex(0);
        }
    });

    connect(selectRegister, &QToolButton::clicked, this, [=] () {
        if (stackLayout->currentIndex() != 1) {
            stackLayout->setCurrentIndex(1);
        }
    });

    auto updateSignBtnState = [=] () {
        auto userName = signNameLineEdit->text().trimmed();
        if (userName.isEmpty()) {
            signBtn->setEnabled(false);
            return ;
        }
        auto userPassword = signPwdLineEdit->text().trimmed();
        if (userPassword.isEmpty()) {
            signBtn->setEnabled(false);
            return ;
        }

        signBtn->setEnabled(true);
    };

    connect(signNameLineEdit, &QLineEdit::textChanged, this, [=] () {
        updateSignBtnState();
    });
    connect(signPwdLineEdit, &QLineEdit::textChanged, this, [=] () {
        updateSignBtnState();
    });

    connect(signBtn, &QPushButton::clicked, this, [=] () {
       auto userName = signNameLineEdit->text().trimmed();
       auto userPassword = signPwdLineEdit->text().trimmed();

       QString password;
       if (this->findUser(userName, password)) {
           qDebug() << "user :" << userName << "    password:" << password;
           if (userPassword == password) {
               qDebug() << "success.";
           } else {
               qDebug() << "failed.User password error.";
           }
       } else {
           qDebug() << "can not find user " << userName;
       }
    });


}

Widget::~Widget()
{
    if (QSqlDatabase::database(kConnectionName).isOpen())
        QSqlDatabase::database(kConnectionName).close();
}

bool Widget::initDb()
{
    if (QSqlDatabase::connectionNames().contains(kConnectionName)) {
        qWarning() << "warning:connection exits " << kConnectionName;
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    if (!db.isValid()) {
        qWarning() << "warning:connection failed " << kConnectionName;
        return false;
    }

    db.setDatabaseName(kDatabaseName);

    if (!db.open()) {
        qWarning() << "warining:open db failed " << kConnectionName;
        return false;
    }

    // user table
    {
        QString userTable(""
                          "CREATE TABLE IF NOT EXISTS test_user_table"
                          "("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                          "name CHAR(50) UNIQUE NOT NULL,"
                          "password CHAR(50) UNIQUE NOT NULL"
                          ");");
        QSqlQuery query(db);
        if (!query.exec(userTable)) {
            qWarning() << "warning:create user table failed";
            return false;
        }
    }

    // test data
    {
        const QString name("testUser");
        const QString password("testPassword");
        insertUser(name, password);
    }


    return true;
}

bool Widget::insertUser(const QString &name, const QString &password)
{
    QSqlQuery query(QSqlDatabase::database(kConnectionName));

    QString insertData = QString(""
                                 "INSERT INTO test_user_table(name, password)"
                                 "VALUES('%1', '%2');").arg(name).arg(password);
    bool result = query.exec(insertData);
    qDebug() << "---------- insert user " << result;
    return result;
}

bool Widget::findUser(const QString &name, QString &password)
{
    QSqlQuery query(QSqlDatabase::database(kConnectionName));

    QString findData = QString("SELECT name,password FROM test_user_table WHERE name='%1';").arg(name);

    if (query.exec(findData)) {
        while (query.next()) {
            qDebug() << "----- find user:" << name << query.value(0).toString() << query.value(1).toString();
            password = query.value(1).toString();
        }

        if (!password.isEmpty())
            return true;
    }

    return false;

}
