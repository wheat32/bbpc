#pragma once

#include <QDialog>

// AboutDialog - shows the app version, the package type, the port's folders, the disclaimer, and
// the credits: the original project bbpc is a fork of, and the components the port is built on.
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
};
