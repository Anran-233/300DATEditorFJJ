#ifndef CPATCH_H
#define CPATCH_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QDebug>

// DAT数据
struct SDatData {
    int         type    { 0 };      // 类型: 0.为空值; 1.整数型; 2.浮点型; 3.字符型;
    long long   Int     { 0 };      // 整数型
    float       Float   { 0.0f };   // 浮点型
    QString     Str     { "" };     // 字符型
};
typedef QVector<QVector<SDatData> > DATLIST;    // DAT数据表

// DPK数据
struct SDpkData {
    int     column  { 0 };  // 所在列号
    int     index1  { 0 };  // 整数索引值1
    int     index2  { 0 };  // 整数索引值2
    QString indexS  { "" }; // 字符索引值
    int     type    { 0 };  // 类型: 0.缺省值; 1.整数型; 2.浮点型; 3.字符型;
    QString data    { "" }; // 储存数据(默认均转换为字符串保存)
};
typedef struct {
    int mode{ 0 };          // 索引模式: 0.首列索引; 1.一二索引; 2.一三索引; 3.字符索引;
    QVector<SDpkData> list; // DPK数据列表
} DPKLIST;  // DPK数据表

// DATPK数据
struct SDatDpk {
    int     row     { 0 };  // 行号
    int     column  { 0 };  // 列号
    int     type    { 0 };  // 类型: 0.缺省值; 1.整数型; 2.浮点型; 3.字符型;
    QString data    { "" }; // 储存数据(默认均转换为字符串保存)
};
typedef QMap<QPair<int, int>, SDatDpk> DATPKLIST;   // DAT与DPK的数据对照表，键值1用来行列索引，键值2用来储存数据

// jmp文件索引表
struct SJmpIndex {
    QString name;       // 文件名
    int index { 0 };    // 索引位置
    int compLen { 0 };  // 压缩数据长度
    int fileLen { 0 };  // 文件数据长度
};
typedef QHash<QString, SJmpIndex> JMPLIST;   // JMP文件

namespace CPatch
{
    // 读取dat数据(返回0为正常运行)(datData 原始数据, datDataLen 原始数据长度, datList 用来存放解析后的DAT数据表)
    int ReadDatData(const char* datData, const int& datDataLen, DATLIST& datList);

    // 写出dat数据(返回0为正常运行)(datList DAT数据表, datData 存放压缩后的dat数据)
    int WriteDatData(DATLIST& datList, QByteArray& datData);

    // 获取dpk索引模式(返回 索引模式: 0.首列索引; 1.一二索引; 2.一三索引; 3.字符索引; -1.索引错误)(datList DAT数据表)
    int GetDpkMode(DATLIST& datList);

    // 读取dpk数据(返回0为正常运行)(dpkData 原始数据, dpkDataLen 原始数据长度, dpkList 用来存放解析后的DPK数据表)
    int ReadDpkData(const char* dpkData, const int& dpkDataLen, DPKLIST& dpkList);

    // 写出dpk数据(返回0为正常运行)(dpkList DPK数据表, dpkData 存放压缩后的dpk数据)
    int WriteDpkData(DPKLIST& dpkList, QByteArray& dpkData);

    // DAT数据导出到DPK数据(返回0为正常运行)(datList DAT数据表, dpkList DPK数据表, datpkList DAT与DPK的数据对照表, mode 索引模式)
    int DatToDpk(DATLIST& datList, DPKLIST& dpkList, DATPKLIST& datpkList, const int& mode);

    // DPK数据导入到DAT数据(返回0为正常运行)(dpkList DPK数据表, datList DAT数据表, datpkList DAT与DPK的数据对照表, mode 索引模式)
    int DpkToDat(DPKLIST& dpkList, DATLIST& datList, DATPKLIST& datpkList, const int& mode);

    // 读取jmp数据中的dat列表(返回0为正常运行)(jmpData 原始数据, jmpDataLen 原始数据长度, jmpList 用来存放DAT文件的数据索引表)
    int ReadJmpDataOfDat(const char* jmpData, const int& jmpDataLen, JMPLIST& jmpList);
};

static const QMap<QString, int> const_datIndexList {
    { "bulletdata_c",       0 },
    { "bulletstatudata_c",  0 },
    { "designation_c",      0 },
    { "hero_c",             0 },
    { "heroskin_c",         1 },
    { "item_item_c",        0 },
    { "monster_c",          0 },
    { "skill_skillbase_c",  1 },
    { "skill_skilllevel_c", 1 },
    { "skill_statuslist_c", 1 },
    { "sound_actsound_c",   0 },
    { "string_client_c",    3 }
};

#endif // CPATCH_H
