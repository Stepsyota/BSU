#include "StatsDialog.h"

#include "JsonStore.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

StatsDialog::StatsDialog(sim::SessionStats stats, QWidget* parent)
    : QDialog(parent)
    , stats_(std::move(stats))
{
    setWindowTitle(QStringLiteral("Статистика сеанса"));
    resize(560, 480);

    auto* layout = new QVBoxLayout(this);
    text_ = new QPlainTextEdit(this);
    text_->setReadOnly(true);
    text_->setPlainText(QString::fromStdString(sim::formatReport(stats_)));
    layout->addWidget(text_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* exportButton = buttons->addButton(QStringLiteral("Экспорт…"), QDialogButtonBox::ActionRole);
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Экспорт отчёта"),
                                                          QStringLiteral("report.txt"),
                                                          QStringLiteral("Текст (*.txt);;Все файлы (*)"));
        if (path.isEmpty()) {
            return;
        }
        QString error;
        if (!io::JsonStore::saveReport(path, stats_, &error)) {
            QMessageBox::warning(this, QStringLiteral("Ошибка"), error);
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
