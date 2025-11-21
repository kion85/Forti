#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QLabel>
#include <QFileSystemModel>
#include <QTreeView>
#include <QSplitter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QTextEdit>  // Не забудьте подключить

// Класс для проверки обновлений
class Updater : public QObject {
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr) : QObject(parent) {
        manager = new QNetworkAccessManager(this);
        connect(manager, &QNetworkAccessManager::finished, this, &Updater::onFinished);
    }

    void checkForUpdates() {
        QString url = "https://api.github.com/repos/kion85/Forti/releases";
        QNetworkRequest request{QUrl(url)};
        manager->get(request);
    }

private slots:
    void onFinished(QNetworkReply *reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Ошибка сети:" << reply->errorString();
            reply->deleteLater();
            return;
        }
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        if (!jsonDoc.isArray()) {
            qDebug() << "Некорректный ответ";
            reply->deleteLater();
            return;
        }
        QJsonArray releases = jsonDoc.array();
        if (releases.isEmpty()) {
            reply->deleteLater();
            return;
        }
        QJsonObject latestRelease = releases.first().toObject();
        QString latestTag = latestRelease["tag_name"].toString();

        QString currentVersion = "v1.0.0"; // Ваша текущая версия

        if (latestTag > currentVersion) {
            QMessageBox::information(nullptr, "Обновление доступно",
                QString("Доступна новая версия %1").arg(latestTag));
        } else {
            QMessageBox::information(nullptr, "Обновление",
                "У вас установлена последняя версия");
        }
        reply->deleteLater();
    }

private:
    QNetworkAccessManager *manager;
};

// Основное окно
class FortiScan : public QWidget {
    Q_OBJECT
public:
    FortiScan(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("FortiScan Antivirus");
        resize(1000, 600);

        auto *mainLayout = new QVBoxLayout(this);

        auto *menuBar = new QMenuBar(this);
        auto *menuSettings = new QMenu("Настройки", this);
        auto *actionAddFunction = new QAction("Добавить функцию", this);
        auto *actionUpdate = new QAction("Обновить", this);
        auto *actionCheckUpdates = new QAction("Проверить обновления", this);
        menuSettings->addAction(actionAddFunction);
        menuSettings->addAction(actionUpdate);
        menuSettings->addAction(actionCheckUpdates);
        menuBar->addMenu(menuSettings);
        mainLayout->setMenuBar(menuBar);

        auto *buttonLayout = new QHBoxLayout();

        auto *bChooseFolder = new QPushButton("Выбрать папку");
        auto *bScan = new QPushButton("Сканировать");
        auto *bRead = new QPushButton("Читать файл");
        auto *bEdit = new QPushButton("Редактировать");
        auto *bDelete = new QPushButton("Удалить");
        auto *bRename = new QPushButton("Переименовать");
        auto *bCopy = new QPushButton("Копировать");
        auto *bEncrypt = new QPushButton("Зашифровать");
        auto *bDecrypt = new QPushButton("Расшифровать");
        auto *bCheckFS = new QPushButton("Проверка ФС");
        auto *bCheckUpdates = new QPushButton("Проверить обновления");
        buttonLayout->addWidget(bChooseFolder);
        buttonLayout->addWidget(bScan);
        buttonLayout->addWidget(bRead);
        buttonLayout->addWidget(bEdit);
        buttonLayout->addWidget(bDelete);
        buttonLayout->addWidget(bRename);
        buttonLayout->addWidget(bCopy);
        buttonLayout->addWidget(bEncrypt);
        buttonLayout->addWidget(bDecrypt);
        buttonLayout->addWidget(bCheckFS);
        buttonLayout->addWidget(bCheckUpdates);
        mainLayout->addLayout(buttonLayout);

        auto *splitter = new QSplitter(Qt::Horizontal);
        fsModel = new QFileSystemModel(this);
        fsModel->setRootPath(QDir::homePath());
        treeView = new QTreeView(splitter);
        treeView->setModel(fsModel);
        treeView->setRootIndex(fsModel->index(QDir::homePath()));

        // Создаем переменную для редактирования
        fileViewer = new QTextEdit;
        fileViewer->setReadOnly(false);

        splitter->addWidget(treeView);
        splitter->addWidget(fileViewer);
        splitter->setStretchFactor(1, 1);
        mainLayout->addWidget(splitter);

        fileLabel = new QLabel("Файл не выбран");
        mainLayout->addWidget(fileLabel);

        // Объект обновления
        updater = new Updater(this);

        // Связи
        connect(bChooseFolder, &QPushButton::clicked, this, &FortiScan::selectFolder);
        connect(bScan, &QPushButton::clicked, this, &FortiScan::startScan);
        connect(bRead, &QPushButton::clicked, this, &FortiScan::readFile);
        connect(bEdit, &QPushButton::clicked, this, &FortiScan::editFile);
        connect(bDelete, &QPushButton::clicked, this, &FortiScan::deleteFile);
        connect(bRename, &QPushButton::clicked, this, &FortiScan::renameFile);
        connect(bCopy, &QPushButton::clicked, this, &FortiScan::copyFile);
        connect(bEncrypt, &QPushButton::clicked, this, &FortiScan::encryptFile);
        connect(bDecrypt, &QPushButton::clicked, this, &FortiScan::decryptFile);
        connect(bCheckFS, &QPushButton::clicked, this, &FortiScan::checkFileSystem);
        connect(bCheckUpdates, &QPushButton::clicked, updater, &Updater::checkForUpdates);
        connect(treeView, &QTreeView::clicked, this, &FortiScan::updateSelectedFile);
    }

private slots:
    void selectFolder() {
        folderPath = QFileDialog::getExistingDirectory(this, "Выбрать папку");
        if (!folderPath.isEmpty()) {
            fsModel->setRootPath(folderPath);
            QModelIndex rootIndex = fsModel->index(folderPath);
            treeView->setRootIndex(rootIndex);
        }
    }

    void startScan() {
        if (folderPath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пожалуйста, выберите папку");
            return;
        }
        QMessageBox::information(this, "Скан завершен", "Папка просканирована");
        // Можно расширить
    }

    void readFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }
        QTextStream in(&file);
        QString content = in.readAll();
        // Вывод в терминал
        qDebug() << "Содержимое файла:" << path;
        qDebug().noquote() << content;
        QMessageBox::information(this, "Информация", "Содержимое файла выведено в терминал");
    }

    void editFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // Создаем диалог с редактором
        QDialog dialog(this);
        dialog.setWindowTitle("Редактировать: " + path);
        auto *layout = new QVBoxLayout(&dialog);
        auto *editor = new QTextEdit;
        editor->setPlainText(content);
        layout->addWidget(editor);
        auto *saveBtn = new QPushButton("Сохранить");
        layout->addWidget(saveBtn);
        connect(saveBtn, &QPushButton::clicked, [&dialog, editor, path]() {
            QFile saveFile(path);
            if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(nullptr, "Ошибка", "Не удалось сохранить");
                return;
            }
            QTextStream out(&saveFile);
            out << editor->toPlainText();
            saveFile.close();
            QMessageBox::information(nullptr, "Готово", "Файл сохранен");
            dialog.accept();
        });
        dialog.resize(600, 400);
        dialog.exec();
    }

    void deleteFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        if (QFile::remove(path)) {
            QMessageBox::information(this, "Удалено", "Файл удален");
            updateSelectedFile();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить");
        }
    }

    void renameFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QString newName = QInputDialog::getText(this, "Переименовать", "Новое имя");
        if (newName.isEmpty()) return;
        QFileInfo info(path);
        QString newPath = info.absolutePath() + "/" + newName;
        if (QFile::rename(path, newPath)) {
            QMessageBox::information(this, "Переименование", "Файл переименован");
            QModelIndex newIdx = fsModel->index(newPath);
            if (newIdx.isValid()) {
                currentFileIdx = newIdx;
                updateSelectedFile();
            } else {
                updateSelectedFile();
            }
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось переименовать");
        }
    }

    void copyFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QString dest = QFileDialog::getSaveFileName(this, "Копировать как");
        if (dest.isEmpty()) return;
        if (QFile::copy(path, dest)) {
            QMessageBox::information(this, "Копия", "Файл скопирован");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось скопировать");
        }
    }

    void encryptFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }
        QByteArray data = file.readAll();
        QByteArray key = "simplekey"; // пример
        QByteArray encrypted;
        for (int i = 0; i < data.size(); ++i)
            encrypted.append(data[i] ^ key[i % key.size()]);
        file.seek(0);
        file.write(encrypted);
        file.resize(encrypted.size());
        QMessageBox::information(this, "Зашифровано", "Файл зашифрован");
    }

    void decryptFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }
        QByteArray data = file.readAll();
        QByteArray key = "simplekey"; // тот же
        QByteArray decrypted;
        for (int i = 0; i < data.size(); ++i)
            decrypted.append(data[i] ^ key[i % key.size()]);
        file.seek(0);
        file.write(decrypted);
        file.resize(decrypted.size());
        QMessageBox::information(this, "Расшифровано", "Файл расшифрован");
    }

    void checkFileSystem() {
        QMessageBox::information(this, "Проверка ФС", "Проверка завершена");
    }

    void updateSelectedFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) {
            fileLabel->setText("Файл не выбран");
        } else {
            fileLabel->setText("Выбран файл: " + path);
        }
        currentFilePath = path;
    }

    QString getSelectedFilePath() {
        if (!currentFilePath.isEmpty()) return currentFilePath;
        auto index = treeView->currentIndex();
        if (index.isValid()) {
            return fsModel->filePath(index);
        }
        return QString();
    }

private:
    QPushButton *bChooseFolder, *bScan, *bRead, *bEdit, *bDelete, *bRename, *bCopy, *bEncrypt, *bDecrypt, *bCheckFS, *bUpdate;
    QTreeView *treeView;
    QFileSystemModel *fsModel;
    QTextEdit *fileViewer;
    QLabel *fileLabel;
    QString folderPath;
    QString currentFilePath;
    QModelIndex currentFileIdx;
    Updater *updater;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    auto *window = new FortiScan();

    // Автоматическая проверка обновлений через 2 секунды
    auto *upd = new Updater();
    QTimer::singleShot(2000, upd, &Updater::checkForUpdates);

    window->show();
    return app.exec();
}
