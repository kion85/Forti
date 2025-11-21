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
#include <QTextEdit>
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDialog>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QCoreApplication>

static const char *APP_VERSION = "v1.0.0";

// Класс для проверки обновлений
class Updater : public QObject {
    Q_OBJECT
public:
    explicit Updater(const QString &currentVersion, QObject *parent = nullptr)
        : QObject(parent)
        , manager(new QNetworkAccessManager(this))
        , m_currentVersion(currentVersion)
    {
        connect(manager, &QNetworkAccessManager::finished,
                this, &Updater::onFinished);
    }

    void checkForUpdates() {
        QString url = "https://api.github.com/repos/kion85/Forti/releases";
        QNetworkRequest request{QUrl(url)};
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          "FortiScan-Client"); // GitHub любит User-Agent
        manager->get(request);
    }

private:
    static void parseVersionString(const QString &str,
                                   int &major, int &minor, int &patch)
    {
        major = minor = patch = 0;
        QString t = str.trimmed();
        if (t.startsWith('v', Qt::CaseInsensitive))
            t.remove(0, 1);

        const auto parts = t.split('.');
        if (parts.size() > 0) major = parts[0].toInt();
        if (parts.size() > 1) minor = parts[1].toInt();
        if (parts.size() > 2) patch = parts[2].toInt();
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

        int latestMaj, latestMin, latestPatch;
        int currentMaj, currentMin, currentPatch;

        parseVersionString(latestTag, latestMaj, latestMin, latestPatch);
        parseVersionString(m_currentVersion, currentMaj, currentMin, currentPatch);

        bool newer = false;
        if (latestMaj > currentMaj) newer = true;
        else if (latestMaj == currentMaj && latestMin > currentMin) newer = true;
        else if (latestMaj == currentMaj && latestMin == currentMin && latestPatch > currentPatch) newer = true;

        QWidget *parentWidget = qobject_cast<QWidget *>(parent());

        if (newer) {
            QMessageBox::information(parentWidget,
                                     "Обновление доступно",
                                     QString("Доступна новая версия %1").arg(latestTag));
        } else {
            QMessageBox::information(parentWidget,
                                     "Обновление",
                                     "У вас установлена последняя версия");
        }
        reply->deleteLater();
    }

private:
    QNetworkAccessManager *manager;
    QString m_currentVersion;
};

// Основное окно
class FortiScan : public QWidget {
    Q_OBJECT
public:
    explicit FortiScan(QWidget *parent = nullptr)
        : QWidget(parent)
        , treeView(new QTreeView(this))
        , fsModel(new QFileSystemModel(this))
        , fileViewer(new QTextEdit(this))
        , fileLabel(new QLabel("Файл не выбран", this))
        , updater(new Updater(QString::fromLatin1(APP_VERSION), this))
    {
        setWindowTitle(QString("FortiScan Antivirus %1")
                           .arg(QCoreApplication::applicationVersion()));
        resize(1000, 600);

        auto *mainLayout = new QVBoxLayout(this);

        // Меню
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

        auto *bChooseFolder  = new QPushButton("Выбрать папку");
        auto *bScan          = new QPushButton("Сканировать");
        auto *bRead          = new QPushButton("Читать файл");
        auto *bEdit          = new QPushButton("Редактировать");
        auto *bDelete        = new QPushButton("Удалить");
        auto *bRename        = new QPushButton("Переименовать");
        auto *bCopy          = new QPushButton("Копировать");
        auto *bEncrypt       = new QPushButton("Зашифровать");
        auto *bDecrypt       = new QPushButton("Расшифровать");
        auto *bCheckFS       = new QPushButton("Проверка ФС");
        auto *bCheckUpdates  = new QPushButton("Проверить обновления");

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

        // Дерево + просмотрщик
        auto *splitter = new QSplitter(Qt::Horizontal);
        fsModel->setRootPath(QDir::homePath());
        treeView->setModel(fsModel);
        treeView->setRootIndex(fsModel->index(QDir::homePath()));

        fileViewer->setReadOnly(true); // лог/просмотр, редактирование через отдельное окно

        splitter->addWidget(treeView);
        splitter->addWidget(fileViewer);
        splitter->setStretchFactor(1, 1);
        mainLayout->addWidget(splitter);

        mainLayout->addWidget(fileLabel);

        // Связи
        connect(bChooseFolder, &QPushButton::clicked,
                this, &FortiScan::selectFolder);
        connect(bScan, &QPushButton::clicked,
                this, &FortiScan::startScan);
        connect(bRead, &QPushButton::clicked,
                this, &FortiScan::readFile);
        connect(bEdit, &QPushButton::clicked,
                this, &FortiScan::editFile);
        connect(bDelete, &QPushButton::clicked,
                this, &FortiScan::deleteFile);
        connect(bRename, &QPushButton::clicked,
                this, &FortiScan::renameFile);
        connect(bCopy, &QPushButton::clicked,
                this, &FortiScan::copyFile);
        connect(bEncrypt, &QPushButton::clicked,
                this, &FortiScan::encryptFile);
        connect(bDecrypt, &QPushButton::clicked,
                this, &FortiScan::decryptFile);
        connect(bCheckFS, &QPushButton::clicked,
                this, &FortiScan::checkFileSystem);
        connect(bCheckUpdates, &QPushButton::clicked,
                updater, &Updater::checkForUpdates);
        connect(treeView, &QTreeView::clicked,
                this, &FortiScan::treeItemClicked);

        // Меню
        connect(actionAddFunction, &QAction::triggered, this, [this]() {
            QMessageBox::information(this,
                                     "Пока заглушка",
                                     "Здесь может быть настройка пользовательских профилей сканирования.\n"
                                     "Сейчас это просто заглушка.");
        });

        connect(actionUpdate, &QAction::triggered,
                this, &FortiScan::openDownloadPage);

        connect(actionCheckUpdates, &QAction::triggered,
                updater, &Updater::checkForUpdates);

        // Автоматическая проверка обновлений через 2 секунды после старта
        QTimer::singleShot(2000, updater, &Updater::checkForUpdates);
    }

private slots:
    void selectFolder() {
        QString dir = QFileDialog::getExistingDirectory(this, "Выбрать папку", folderPath.isEmpty() ? QDir::homePath() : folderPath);
        if (!dir.isEmpty()) {
            folderPath = dir;
            fsModel->setRootPath(folderPath);
            QModelIndex rootIndex = fsModel->index(folderPath);
            treeView->setRootIndex(rootIndex);
            fileViewer->clear();
            fileLabel->setText("Файл не выбран");
            currentFilePath.clear();
        }
    }

    void startScan() {
        if (folderPath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пожалуйста, выберите папку для сканирования");
            return;
        }

        QStringList suspiciousExtensions = { "exe", "dll", "scr", "bat", "cmd", "js", "vbs" };
        QStringList suspiciousFiles;

        int totalFiles = 0;

        QProgressDialog progress("Сканирование...", "Отмена", 0, 0, this);
        progress.setWindowModality(Qt::ApplicationModal);
        progress.show();

        QDirIterator it(folderPath,
                        QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            ++totalFiles;

            QFileInfo info(filePath);
            QString ext = info.suffix().toLower();
            if (suspiciousExtensions.contains(ext)) {
                suspiciousFiles << filePath;
            }

            if (totalFiles % 200 == 0) {
                progress.setLabelText(QString("Проверено файлов: %1").arg(totalFiles));
                qApp->processEvents();
                if (progress.wasCanceled())
                    break;
            }
        }

        progress.close();

        fileViewer->clear();
        fileViewer->append(QString("Сканирование папки: %1\n").arg(folderPath));
        fileViewer->append(QString("Всего файлов: %1").arg(totalFiles));
        fileViewer->append(QString("Подозрительных: %1").arg(suspiciousFiles.size()));
        fileViewer->append("\n");

        if (!suspiciousFiles.isEmpty()) {
            fileViewer->append("Подозрительные файлы:");
            for (const QString &f : suspiciousFiles) {
                fileViewer->append(" - " + f);
            }
        } else {
            fileViewer->append("Подозрительных файлов не найдено.");
        }

        QMessageBox::information(this,
                                 "Сканирование завершено",
                                 "Проверка папки завершена. Результат в правом окне.");
    }

    void readFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Файл не выбран");
            return;
        }
        loadFileToViewer(path);
        QMessageBox::information(this, "Информация",
                                 "Содержимое файла выведено в правое окно.");
    }

    void editFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;

        QFileInfo info(path);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выбран не файл");
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // Диалог с редактором
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
                QMessageBox::warning(&dialog, "Ошибка", "Не удалось сохранить");
                return;
            }
            QTextStream out(&saveFile);
            out << editor->toPlainText();
            saveFile.close();
            QMessageBox::information(&dialog, "Готово", "Файл сохранен");
            dialog.accept();
        });

        dialog.resize(600, 400);
        if (dialog.exec() == QDialog::Accepted) {
            // Обновить просмотр
            // (если именно этот файл открыт в правом окне)
            loadFileToViewer(path);
        }
    }

    void deleteFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;

        QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выберите существующий файл");
            return;
        }

        auto res = QMessageBox::question(
            this,
            "Удаление",
            QString("Удалить файл?\n%1").arg(path)
        );
        if (res != QMessageBox::Yes)
            return;

        if (QFile::remove(path)) {
            QMessageBox::information(this, "Удалено", "Файл удален");
            currentFilePath.clear();
            fileLabel->setText("Файл не выбран");
            fileViewer->clear();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить");
        }
    }

    void renameFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;

        QFileInfo info(path);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выберите файл");
            return;
        }

        QString newName = QInputDialog::getText(this,
                                                "Переименовать",
                                                "Новое имя",
                                                QLineEdit::Normal,
                                                info.fileName());
        if (newName.isEmpty()) return;

        QString newPath = info.absolutePath() + "/" + newName;
        if (QFile::rename(path, newPath)) {
            QMessageBox::information(this, "Переименование", "Файл переименован");
            currentFilePath = newPath;
            fileLabel->setText("Выбран файл: " + newPath);
            treeView->setCurrentIndex(fsModel->index(newPath));
            loadFileToViewer(newPath);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось переименовать");
        }
    }

    void copyFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;

        QFileInfo info(path);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выберите файл");
            return;
        }

        QString dest = QFileDialog::getSaveFileName(this,
                                                    "Копировать как",
                                                    info.fileName());
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

        QFileInfo info(path);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выберите файл для шифрования");
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QByteArray key = "simplekey"; // простой XOR-ключ для примера
        QByteArray encrypted;
        encrypted.reserve(data.size());
        for (int i = 0; i < data.size(); ++i)
            encrypted.append(data[i] ^ key[i % key.size()]);

        QString outPath = path + ".enc";
        QFile outFile(outPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать зашифрованный файл");
            return;
        }
        outFile.write(encrypted);
        outFile.close();

        QMessageBox::information(this,
                                 "Зашифровано",
                                 QString("Файл зашифрован в:\n%1").arg(outPath));
    }

    void decryptFile() {
        QString path = getSelectedFilePath();
        if (path.isEmpty()) return;

        QFileInfo info(path);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Ошибка", "Выберите файл для расшифровки");
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QByteArray key = "simplekey";
        QByteArray decrypted;
        decrypted.reserve(data.size());
        for (int i = 0; i < data.size(); ++i)
            decrypted.append(data[i] ^ key[i % key.size()]);

        QString outPath = path;
        if (outPath.endsWith(".enc"))
            outPath.chop(4);
        else
            outPath += ".dec";

        QFile outFile(outPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось создать расшифрованный файл");
            return;
        }
        outFile.write(decrypted);
        outFile.close();

        QMessageBox::information(this,
                                 "Расшифровано",
                                 QString("Файл расшифрован в:\n%1").arg(outPath));
    }

    void checkFileSystem() {
        if (folderPath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пожалуйста, выберите папку");
            return;
        }

        qint64 totalSize = 0;
        int dirCount = 0;
        int fileCount = 0;

        QDirIterator it(folderPath,
                        QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo info = it.fileInfo();
            if (info.isDir())
                ++dirCount;
            else if (info.isFile()) {
                ++fileCount;
                totalSize += info.size();
            }
        }

        double sizeMb = totalSize / 1024.0 / 1024.0;

        QString infoText = QString(
            "Папка: %1\n"
            "Каталогов: %2\n"
            "Файлов: %3\n"
            "Общий размер файлов: %4 МБ\n")
                .arg(folderPath)
                .arg(dirCount)
                .arg(fileCount)
                .arg(sizeMb, 0, 'f', 2);

        fileViewer->setPlainText(infoText);
        QMessageBox::information(this,
                                 "Проверка ФС",
                                 "Информация по файловой системе выведена в правое окно.");
    }

    void treeItemClicked(const QModelIndex &index) {
        QString path = fsModel->filePath(index);
        currentFilePath = path;

        QFileInfo info(path);
        if (info.isFile())
            fileLabel->setText("Выбран файл: " + path);
        else
            fileLabel->setText("Выбран каталог: " + path);

        if (info.isFile())
            loadFileToViewer(path);
        else
            fileViewer->clear();
    }

    void openDownloadPage() {
        QDesktopServices::openUrl(
            QUrl("https://github.com/kion85/Forti/releases/latest"));
    }

private:
    QString getSelectedFilePath() const {
        if (!currentFilePath.isEmpty())
            return currentFilePath;

        QModelIndex index = treeView->currentIndex();
        if (index.isValid())
            return fsModel->filePath(index);

        return QString();
    }

    void loadFileToViewer(const QString &path) {
        QFileInfo info(path);
        if (!info.isFile()) {
            fileViewer->setPlainText("Это каталог, а не файл.");
            return;
        }

        // Ограничим размер файла для просмотра, чтобы не повесить приложение
        const qint64 maxPreviewSize = 2 * 1024 * 1024; // 2 МБ
        if (info.size() > maxPreviewSize) {
            fileViewer->setPlainText("Файл слишком большой для просмотра (более 2 МБ).");
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fileViewer->setPlainText("Не удалось открыть файл для чтения.");
            return;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        fileViewer->setPlainText(content);
    }

private:
    QTreeView *treeView;
    QFileSystemModel *fsModel;
    QTextEdit *fileViewer;
    QLabel *fileLabel;
    QString folderPath;
    QString currentFilePath;
    Updater *updater;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("FortiScan Antivirus");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    FortiScan window;
    window.show();

    return app.exec();
}
