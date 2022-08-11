#include "cpatch.h"

#include <QSet>

int CPatch::ReadDatData(const char *datData, const int &datDataLen, DATLIST &datList)
{
    const int&& int_max{ 0x3fffffff };
    int&& index{ 0 };
    datList.clear();

    /* 是否读取到结尾 */ auto lam_isend = [&](const int& i) -> bool {
        if (i < datDataLen) return false;
        if (i != datDataLen) index = int_max;
        return true;
    };
    /* 获取数据类型 */ auto lam_rtype = [](const char& data_c) -> int {
        char&& type_c = data_c & 0x07;
        if (type_c == 0x00) return 1;
        if (type_c == 0x05) return 2;
        if (type_c == 0x02) return 3;
        return 0;
    };
    /* 读取列号 */ auto lam_rcolumn = [&]() -> long long {
        int&& n{ 0 };
        long long&& nColumn{ 0 };
        do {
            if (lam_isend(index + n)) return 0;
            if (n == 0) nColumn |= (long long)(datData[index] & 0x78) >> 3;
            else        nColumn |= (long long)(datData[index + n] & 0x7f) << (7 * n - 3);
            n += 1;
        } while ((datData[index + n - 1] & 0x80) != 0);
        index += n;
        return nColumn;
    };
    /* 读取vint数据 */ auto lam_rvint = [&]() -> long long {
        int&& n{ 0 };
        long long&& nValue{ 0 };
        do {
            if (lam_isend(index + n)) return 0;
            nValue |= (long long)(datData[index + n] & 0x7f) << (7 * n);
            n += 1;
        } while ((datData[index + n - 1] & 0x80) != 0);
        index += n;
        return nValue;
    };
    /* 读取float数据 */ auto lam_rfloat = [&]() -> float {
        if (lam_isend(index + 3)) return 0.0f;
        float&& f{ 0.0f };
        for (int&& i = 0; i < 4; i++, index++) *((char*)&f + i) = datData[index];
        return f;
    };
    /* 读取str数据 */ auto lam_rstr = [&]() -> QString {
        int&& nStrLen = (int)lam_rvint();
        const char* data = &datData[index];
        index += nStrLen;
        if (lam_isend(index - 1)) return QString("");
        return QString::fromLocal8Bit(data, nStrLen);
    };

    // 读取行索引表
    QVector<int> lineIndexList;
    while (!lam_isend(index))
    {
        if (datData[index++] != char(0x0a)) return -1;
        int nLineLen = (int)lam_rvint();
        lineIndexList.push_back(index);
        lineIndexList.push_back(index += nLineLen);
    }
    if (index == int_max) return -1;

    // 读取每行数据
    int&& nColumnMax{ 0 };
    datList.resize(lineIndexList.size() / 2);
    for (int&& nRowNum{ 0 }; nRowNum < datList.size(); nRowNum++)
    {
        index = lineIndexList[nRowNum * 2];
        int& indexEnd = lineIndexList[nRowNum * 2 + 1];
        while (index < indexEnd)
        {
            // 读取单个数据
            int&& type = lam_rtype(datData[index]);
            int nColumnNum = (int)lam_rcolumn();
            if (nColumnNum > datList[nRowNum].size())
            {
                datList[nRowNum].resize(nColumnNum);
                datList[nRowNum].back().type = type;
                if (type == 1)      datList[nRowNum].back().Int = lam_rvint();
                else if (type == 2) datList[nRowNum].back().Float = lam_rfloat();
                else if (type == 3) datList[nRowNum].back().Str = lam_rstr();
            }
            else index = int_max;
        }
        if (datList[nRowNum].size() > nColumnMax) nColumnMax = datList[nRowNum].size();
        if (lam_isend(index)) break;
    }
    if (index == int_max) datList.clear();
    else for (auto& datLine : datList) datLine.resize(nColumnMax);
    return index == int_max ? -1 : 0;
}

int CPatch::WriteDatData(DATLIST &datList, QByteArray &datData)
{
    /* 写入列号和类型 */ auto lam_wcolumntype = [](QByteArray& data, const int& nColumnNum, const int& type) {
        int n = nColumnNum;
        if (type == 1) data.push_back(((n & 0x0f) << 3) | 0x00);
        else if (type == 2) data.push_back(((n & 0x0f) << 3) | 0x05);
        else if (type == 3) data.push_back(((n & 0x0f) << 3) | 0x02);
        n = n >> 4;
        while (n)
        {
            data.back() = data.back() | 0x80;
            data.push_back(n & 0x7f);
            n = n >> 7;
        }
    };
    /* 写入vint数据 */ auto lam_wvint = [](QByteArray& data, const long long& value) {
        unsigned long long n = *((unsigned long long*)&value);
        data.push_back(n & 0x7f);
        n = n >> 7;
        while (n)
        {
            data.back() = data.back() | 0x80;
            data.push_back(n & 0x7f);
            n = n >> 7;
        }
    };
    /* 写入float数据 */ auto lam_wfloat = [](QByteArray& data, const float& value) {
        for (int&& i = 0; i < 4; i++) data.push_back(((char*)(&value))[i]);
    };
    /* 写入str数据 */ auto lam_wstr = [&](QByteArray& data, const QString& str) {
        QByteArray&& strLocal8Bit = str.toLocal8Bit();
        lam_wvint(data, (long long)strLocal8Bit.size());
        data.append(strLocal8Bit);
    };

    datData.clear();
    for (auto& datLine : datList)
    {
        QByteArray lineData;
        for(int&& nColumnNum{ 0 }; nColumnNum < datLine.size(); nColumnNum++)
        {
            if (datLine[nColumnNum].type == 0) continue;
            lam_wcolumntype(lineData, nColumnNum + 1, datLine[nColumnNum].type);
            if (datLine[nColumnNum].type == 1) lam_wvint(lineData, datLine[nColumnNum].Int);
            else if (datLine[nColumnNum].type == 2) lam_wfloat(lineData, datLine[nColumnNum].Float);
            else if (datLine[nColumnNum].type == 3) lam_wstr(lineData, datLine[nColumnNum].Str);
        }
        datData.push_back(0x0a);
        lam_wvint(datData, (long long)lineData.size());
        datData.append(lineData);
    }
    return 0;
}

int CPatch::GetDpkMode(DATLIST &datList)
{
    if (datList.size() == 0) return -2;
    if (datList[0].size() == 0) return -2;
    if (datList[0][0].type == 3) return 3;  // 字符索引

    if (datList[0][0].type != 1) return -1;
    QSet<long long> columnList_1;
    columnList_1.reserve(datList.size() + 1);
    for (auto& datLine : datList) columnList_1.insert(datLine[0].Int);
    if (columnList_1.size() == datList.size()) return 0;   // 首列索引

    if (datList[0].size() < 2) return -1;
    if (datList[0][1].type == 1)
    {
        QSet<QPair<long long, long long> > columnList_2;
        columnList_2.reserve(datList.size() + 1);
        for (auto& datLine : datList) columnList_2.insert({ datLine[0].Int, datLine[1].Int });
        if (columnList_2.size() == datList.size()) return 1;   // 一二索引
    }

    if (datList[0].size() < 3) return -1;
    if (datList[0][2].type == 1)
    {
        QSet<QPair<long long, long long> > columnList_3;
        columnList_3.reserve(datList.size() + 1);
        for (auto& datLine : datList) columnList_3.insert({ datLine[0].Int, datLine[2].Int });
        if (columnList_3.size() == datList.size()) return 2;   // 一三索引
    }

    return -1;
}

int CPatch::ReadDpkData(const char *dpkData, const int &dpkDataLen, DPKLIST &dpkList)
{
    if (dpkDataLen < 12) return -1;
    if (QString::fromLocal8Bit(dpkData, 3) != "DPK") return -2;
    int& mode = *((int*)(&dpkData[4]));
    if (mode > 3 || mode < 0) return -3;
    int& nDpkNum = *((int*)(&dpkData[8]));
    const int&& int_max{ 0x3fffffff };
    int&& index{ 12 };

    /* 是否读取到结尾 */ auto lam_isend = [&](const int& i) -> bool {
        if (i < dpkDataLen) return false;
        if (i != dpkDataLen) index = int_max;
        return true;
    };
    /* 读取索引信息 */ auto lam_rindex = [&](SDpkData& data) {
        if (mode == 0) {
            if (lam_isend(index + 7)) return;
            data.column = *((int*)(&dpkData[index]));
            data.index1 = *((int*)(&dpkData[index + 4]));
            index += 8;
        }
        else if (mode == 1) {
            if (lam_isend(index + 11)) return;
            data.column = *((int*)(&dpkData[index]));
            data.index1 = *((int*)(&dpkData[index + 4]));
            data.index2 = *((int*)(&dpkData[index + 8]));
            index += 12;
        }
        else if (mode == 2) {
            if (lam_isend(index + 11)) return;
            data.column = *((int*)(&dpkData[index]));
            data.index1 = *((int*)(&dpkData[index + 4]));
            data.index2 = *((int*)(&dpkData[index + 8]));
            index += 12;
        }
        else if (mode == 3) {
            if (lam_isend(index + 7)) return;
            data.column = *((int*)(&dpkData[index]));
            int strLen = *((int*)(&dpkData[index + 4]));
            if (lam_isend(index + 7 + strLen)) return;
            data.indexS = QString::fromLocal8Bit(&dpkData[index + 8], strLen);
            index = index + 8 + strLen;
        }
    };
    /* 读取储存数据 */ auto lam_rdata = [&](SDpkData& data) {
        if (lam_isend(index)) return;
        data.type = (int)dpkData[index++] + 1;
        if (data.type == 1) {
            if (lam_isend(index + 3)) return;
            data.data = QString::number(*((int*)(&dpkData[index])));
            index += 4;
        }
        else if (data.type == 2) {
            if (lam_isend(index + 3)) return;
            data.data = QString::number(*((float*)(&dpkData[index])));
            index += 4;
        }
        else if (data.type == 3) {
            if (lam_isend(index + 3)) return;
            int strLen = *((int*)(&dpkData[index]));
            if (lam_isend(index + 3 + strLen)) return;
            data.data = QString::fromLocal8Bit(&dpkData[index + 4], strLen);
            index = index + 4 + strLen;
        }
        else index = int_max;
    };

    dpkList.mode = mode;
    dpkList.list.resize(nDpkNum);
    for (auto& data : dpkList.list)
    {
        lam_rindex(data);
        lam_rdata(data);
        if (lam_isend(index)) break;
    }

    if (index != int_max) return 0;
    dpkList.list.clear();
    return -4;
}

int CPatch::WriteDpkData(DPKLIST &dpkList, QByteArray &dpkData)
{
    int nSize;
    dpkData.clear();
    dpkData.append("DPK", 4);
    dpkData.append((char*)&dpkList.mode, 4);
    dpkData.append((char*)&(nSize = dpkList.list.size()), 4);

    /* 写入索引信息 */ auto lam_windex = [&](SDpkData& data) {
        if (dpkList.mode == 0) {
            dpkData.append((char*)&data.column, 4);
            dpkData.append((char*)&data.index1, 4);
        }
        else if (dpkList.mode == 1) {
            dpkData.append((char*)&data.column, 4);
            dpkData.append((char*)&data.index1, 4);
            dpkData.append((char*)&data.index2, 4);
        }
        else if (dpkList.mode == 2) {
            dpkData.append((char*)&data.column, 4);
            dpkData.append((char*)&data.index1, 4);
            dpkData.append((char*)&data.index2, 4);
        }
        else if (dpkList.mode == 3) {
            dpkData.append((char*)&data.column, 4);
            QByteArray&& str = data.indexS.toLocal8Bit();
            dpkData.append((char*)&(nSize = str.size()), 4);
            dpkData.append(str);
        }
    };
    /* 读取储存数据 */ auto lam_wdata = [&](SDpkData& data) {
        dpkData.push_back(char(data.type - 1));
        if (data.type == 1) {
            int&& n = data.data.toInt();
            dpkData.append((char*)&n, 4);
        }
        else if (data.type == 2) {
            float&& f = data.data.toFloat();
            dpkData.append((char*)&f, 4);
        }
        else if (data.type == 3) {
            QByteArray&& str = data.data.toLocal8Bit();
            dpkData.append((char*)&(nSize = str.size()), 4);
            dpkData.append(str);
        }
    };

    for (auto& data : dpkList.list)
    {
        lam_windex(data);
        lam_wdata(data);
    }
    return 0;
}

int CPatch::DatToDpk(DATLIST &datList, DPKLIST &dpkList, DATPKLIST &datpkList, const int &mode)
{
    if (datList.size() == 0) return -1;
    if (datList[0].size() < 2) return -1;
    if (mode == 2 && datList[0].size() < 3) return -1;
    dpkList.mode = mode;
    if (mode == 0)
        for (auto& datpk : datpkList)
            dpkList.list.push_back({ datpk.column, (int)datList[datpk.row][0].Int, 0, "", datpk.type, datpk.data });
    else if (mode == 1)
        for (auto& datpk : datpkList)
            dpkList.list.push_back({ datpk.column, (int)datList[datpk.row][0].Int, (int)datList[datpk.row][1].Int, "", datpk.type, datpk.data });
    else if (mode == 2)
        for (auto& datpk : datpkList)
            dpkList.list.push_back({ datpk.column, (int)datList[datpk.row][0].Int, (int)datList[datpk.row][2].Int, "", datpk.type, datpk.data });
    else if (mode == 3)
        for (auto& datpk : datpkList)
            dpkList.list.push_back({ datpk.column, 0, 0, datList[datpk.row][0].Str, datpk.type, datpk.data });
    else return -2;
    return 0;
}

int CPatch::DpkToDat(DPKLIST &dpkList, DATLIST &datList, DATPKLIST &datpkList, const int &mode)
{
    if (datList.size() == 0) return -1;
    if (datList[0].size() < 2) return -1;
    if (mode == 2 && datList[0].size() < 3) return -1;
    if (mode == 0)
        for (auto& dpk : dpkList.list)
            for (int &&row{ 0 }, &&size{datList.size()}; row < size; ++row)
                if (datList[row][0].Int == dpk.index1) datpkList.insert({ row, dpk.column }, { row, dpk.column, dpk.type, dpk.data });
    if (mode == 1)
        for (auto& dpk : dpkList.list)
            for (int &&row{ 0 }, &&size{datList.size()}; row < size; ++row)
                if (datList[row][0].Int == dpk.index1 && datList[row][1].Int == dpk.index2) datpkList.insert({ row, dpk.column }, { row, dpk.column, dpk.type, dpk.data });
    if (mode == 2)
        for (auto& dpk : dpkList.list)
            for (int &&row{ 0 }, &&size{datList.size()}; row < size; ++row)
                if (datList[row][0].Int == dpk.index1 && datList[row][2].Int == dpk.index2) datpkList.insert({ row, dpk.column }, { row, dpk.column, dpk.type, dpk.data });
    if (mode == 3)
        for (auto& dpk : dpkList.list)
            if (dpk.indexS.size() != 0)
                for (int &&row{ 0 }, &&size{datList.size()}; row < size; ++row)
                    if (datList[row][0].Str == dpk.indexS) datpkList.insert({ row, dpk.column }, { row, dpk.column, dpk.type, dpk.data });
    return 0;
}

int CPatch::ReadJmpDataOfDat(const char *jmpData, const int &jmpDataLen, JMPLIST &jmpList)
{
    if (jmpDataLen < 10485760) return -1;
    int& jmpNum = *((int*)(&jmpData[50]));
    if (jmpNum < 1) return -2;
    jmpList.clear();
    for (int&& i{ 0 }; i < jmpNum; ++i)
    {
        QString strPatchPath(&jmpData[i * 304 + 54]);
        if (strPatchPath.right(4) != ".dat") continue;
        int&& pos = strPatchPath.lastIndexOf('\\');
        if (pos < 0) continue;
        QString&& strName = strPatchPath.mid(pos + 1);
        jmpList.insert(strName, { strName, *((int *)(&jmpData[i * 304 + 314])), *((int *)(&jmpData[i * 304 + 318])), *((int *)(&jmpData[i * 304 + 322])) });
    }
    return 0;
}
