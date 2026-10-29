#pragma once

#include <QDialog>
#include <QString>

// UpdateAvailableDialog - shown when the check at start (UpdateCheck) finds a newer bbpc: both
// versions, and a button to the releases page.
class UpdateAvailableDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateAvailableDialog(const QString& currentVersion, const QString& newVersion, QWidget* parent = nullptr);
};
