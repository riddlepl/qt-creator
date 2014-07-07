    QString errorString;
    Utils::TextFileFormat format;
    format.codec = Core::EditorManager::defaultTextCodec();

    QString leftText;
    if (Utils::TextFileFormat::readFile(m_leftFileName,
                                    format.codec,
                                    &leftText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {

        return;
    }

    QString rightText;
    if (Utils::TextFileFormat::readFile(m_rightFileName,
                                    format.codec,
                                    &rightText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {

        return;
    }
    if (controller()->isIgnoreWhitespace()) {
                chunkData, controller()->contextLinesNumber(), 0);
    controller()->setDiffFiles(fileDataList);
    QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
    document = DiffEditorManager::findOrCreate(documentId, title);
    if (!document)
        return;
    DiffEditorController *controller = document->controller();
    if (!controller->reloader()) {
        controller->setReloader(reloader);
                                "diff --git a/file a.txt b/file b.txt\n"
                                "similarity index 99%\n"
                                "copy from file a.txt\n"
                                "copy to file b.txt\n"
                                "index 1234567..9876543\n"
                                "--- a/file a.txt\n"
                                "+++ b/file b.txt\n"
                                "@@ -20,3 +20,3 @@\n"
                                " A\n"
                                "-B\n"
                                "+C\n"
                                " D\n"
                                "diff --git a/file a.txt b/file b.txt\n"
                                "similarity index 99%\n"
                                "rename from file a.txt\n"
                                "rename to file b.txt\n"
                                );
    fileData3.fileOperation = FileData::NewFile;
    fileData4.fileOperation = FileData::DeleteFile;
    fileData5.fileOperation = FileData::NewFile;
    fileData6.fileOperation = FileData::DeleteFile;

    FileData fileData7;
    fileData7.leftFileInfo = DiffFileInfo(QLatin1String("file a.txt"), QLatin1String("1234567"));
    fileData7.rightFileInfo = DiffFileInfo(QLatin1String("file b.txt"), QLatin1String("9876543"));
    fileData7.fileOperation = FileData::CopyFile;
    ChunkData chunkData7;
    chunkData7.leftStartingLineNumber = 19;
    chunkData7.rightStartingLineNumber = 19;
    QList<RowData> rows7;
    rows7.append(RowData(TextLineData(QLatin1String("A"))));
    rows7.append(RowData(TextLineData(QLatin1String("B")),
                         TextLineData(QLatin1String("C"))));
    rows7.append(RowData(TextLineData(QLatin1String("D"))));
    chunkData7.rows = rows7;
    fileData7.chunks.append(chunkData7);

    FileData fileData8;
    fileData8.leftFileInfo = DiffFileInfo(QLatin1String("file a.txt"));
    fileData8.rightFileInfo = DiffFileInfo(QLatin1String("file b.txt"));
    fileData8.fileOperation = FileData::RenameFile;
    fileDataList << fileData1 << fileData2 << fileData3 << fileData4 << fileData5 << fileData6 << fileData7 << fileData8;
        QCOMPARE(resultFileData.fileOperation, origFileData.fileOperation);