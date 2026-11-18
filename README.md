Лабораторна робота №4
Тема: Lab 04 - Форма керування: умови, цикли, область видимості

Студент: Свириденко Павло Володимирович
Група: Б-F2-26-2-ПІ

1. Інструкції ISA
Опкод	Мнемоніка	Розмір	Прапорці	Що робить
0x1A	CMP A, B	1	Z N C	рахує A - B, виставляє прапорці, результат викидає
0x2D	HLOW	1	-	A = H & 0xFF
0x30	JMP addr16	3	-	PC = addr
0x31	JZ addr16	3	-	PC = addr, якщо Z виставлений
0x32	JNZ addr16	3	-	PC = addr, якщо Z не виставлений

Коли стрибок спрацював, PC = addr і розмір не додається (після цього в step стоїть return). Інакше PC += 3. 

2. step як switch, невідомий опкод і ліміт кроків
Кожен відомий опкод має свій case, у default друкується помилка і машина зупиняється.
ember> set 0 0xFF
ember> run
error: unknown opcode 0xff (PC=0)

run робить не більше 100000 кроків. Перевірка програмою JMP на саму себе:
ember> load 0 0x30 0 0
ember> run
step limit reached, PC=0

3. Зворотний відлік
адр.  байти       текст
0000  20 03       LOADI A, 3
0002  03          loop:  OUTN
0003  19                 DEC A
0004  32 02 00           JNZ loop
0007  00                 HALT

ember> load 0 0x20 3 0x03 0x19 0x32 2 0 0
ember> run
3 2 1

Трасування (стан після кроку):
крок  PC до  інструкція          A  Z  PC після
1     0000   LOADI A, 3          3  0  0002
2     0002   OUTN (3)            3  0  0003
3     0003   DEC A               2  0  0004
4     0004   JNZ (стрибок)       2  0  0002
5     0002   OUTN (2)            2  0  0003
6     0003   DEC A               1  0  0004
7     0004   JNZ (стрибок)       1  0  0002
8     0002   OUTN (1)            1  0  0003
9     0003   DEC A               0  1  0004
10    0004   JNZ (не спрацював)  0  1  0007
11    0007   HALT                0  1  0008

Те саме на C++:
int a = 3;
do {
    std::cout << a << ' ';
    --a;
} while (a != 0);

4. Лінійний пошук на C++
std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) == needle) return i;
    }
    return hi;
}

ember> set 0x805 0x41
ember> find 0x800 0x808 0x41
found at 2053
ember> find 0x800 0x805 0x41
not found

5. Лінійний пошук усередині ember
H це i, INCH це ++i, CMP це ==, JZ це if. Шукає 0x41 у восьми байтах з 0x800.
адр.  байти       текст
0000  28 00 08    LOADH H, 0x0800
0003  29          loop:  LOAD A, [H]
0004  21 41              LOADI B, 0x41
0006  1A                 CMP A, B
0007  31 16 00           JZ found
000A  2B                 INCH
000B  2D                 HLOW
000C  21 08              LOADI B, 0x08
000E  1A                 CMP A, B
000F  32 03 00           JNZ loop
0012  20 FF              LOADI A, 0xFF
0014  03                 OUTN
0015  00                 HALT
0016  2D          found: HLOW
0017  03                 OUTN
0018  00                 HALT

ember> set 0x805 0x41
ember> reset
ember> load 0 0x28 0 8 0x29 0x21 0x41 0x1A 0x31 0x16 0 0x2B 0x2D 0x21 8 0x1A 0x32 3 0 0x20 0xFF 3 0 0x2D 3 0
ember> run
5
ember> set 0x805 0
ember> reset
ember> run
255
