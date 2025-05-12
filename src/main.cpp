#include <array>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <print>
#include <queue>
#include <ranges>
#include <source_location>

#include "LS77_compress.hpp"
#include "bmp.hpp"
#include "dct.hpp"
#include "huffman_coding.hpp"
#include "matrix.hpp"
#include "matrix_view.hpp"

#ifdef WIN32
#include "platform/windows_show.hpp"
#endif

using namespace f9ay;

void test_Matrix() {
    Matrix<std::tuple<int, int, int>> mtx(3, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx[i, j] = std::make_tuple(1, 2, 3);
        }
    }
    std::print("{}", mtx);
    std::println("====================");
    auto view = Matrix_view_fixed<1>(mtx);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view[i, j] << " ";
        }
        std::cout << std::endl;
    }
    std::println("====================");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::println("====================");
    for (const auto& row : view) {
        for (const auto& col : row) {
            std::cout << col << " ";
        }
        std::cout << std::endl;
    }
    /////////////////////////////
    /// rumtime///////////////////
    /////////////////////////////
    std::println("====================");
    auto mtx2 = Matrix<std::array<int, 3>>(3, 3);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            mtx2[i, j][2] = -3;
        }
    }

    auto view2 = Matrix_view(mtx2, 2);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << view2[i, j] << " ";
        }
        std::cout << std::endl;
    }

    std::println("====================");
    /*
    [  [16,  11,  10,  16,  24,  40,  51,  61],
    [12,  12,  14,  19,  26,  58,  60,  55],
    [14,  13,  16,  24,  40,  57,  69,  56],
    [14,  17,  22,  29,  51,  87,  80,  62],
    [18,  22,  37,  56,  68, 109, 103,  77],
    [24,  35,  55,  64,  81, 104, 113,  92],
    [49,  64,  78,  87, 103, 121, 120, 101],
    [72,  92,  95,  98, 112, 100, 103,  99] ]
    */

    Matrix<float> mtx3 = {{16, 11, 10, 16, 24, 40, 51, 61},
                          {12, 12, 14, 19, 26, 58, 60, 55},
                          {14, 13, 16, 24, 40, 57, 69, 56},
                          {14, 17, 22, 29, 51, 87, 80, 62},
                          {18, 22, 37, 56, 68, 109, 103, 77},
                          {24, 35, 55, 64, 81, 104, 113, 92},
                          {49, 64, 78, 87, 103, 121, 120, 101},
                          {72, 92, 95, 98, 112, 100, 103, 99}};
    const auto view3 = Matrix_view(mtx3);

    auto result = Dct<8, 8>::dct(view3);

    std::println("{}", result);

    std::println("======================================");

    Matrix<float> mtx4(8, 8);

    for (auto x : mtx4) {
        for (auto& y : x) {
            y = 128;
        }
    }

    const auto view4 = Matrix_view(mtx4);

    auto res2 = Dct<8, 8>::dct(view4);

    std::print("{}", res2);
    std::println("======================================");
}

int main(int argc, char** argv) {
    // std::filesystem::path path = std::source_location::current().file_name();
    // path = path.parent_path().parent_path() / "test_data" / "fire.bmp";
    // std::cout << path << std::endl;
    // std::ifstream fs(path, std::ios::binary);
    // if (!fs.is_open()) {
    //     std::cerr << "Failed to open file" << std::endl;
    //     return 1;
    // }
    // const auto file = readFile(fs);
    // auto result = Bmp().import(file.get());
    // std::cout << "done" << std::endl;

    // const auto str = "abababa";

    // std::vector<int> data;
    // for (int i = 0; i < 5; i++) {
    //     data.push_back(20);
    // }
    // for (int i = 0; i < 6; i++) {
    //     data.push_back(60);
    // }
    // for (int i = 0; i < 25; i++) {
    //     data.push_back(100);
    // }
    // for (int i = 0; i < 16; i++) {
    //     data.push_back(140);
    // }

    // for (int i = 0; i < 9; i++) {
    //     data.push_back(180);
    // }
    // for (int i = 0; i < 3; i++) {
    //     data.push_back(220);
    // }
    std::string data =
        R"(# Python 物件導向程式設計
## 第一章 物件導向基礎概念
### 類別是什麼
**類別**是一種定義物件的藍圖，你可以透過這個藍圖，寫出各種有不同屬性的物件出來。
例如在python裡面，numbers及list本身就是一個類別，你可以透過a = 15創造一個類別為int的物件a出來。
像是下列程式碼

```python=
class School: #類別第一個字通常為大寫，讓別人知道這是類別
    def __init__(self, name, location):
        self.name = name
        self.location = location
    
ntpu = School("National Taipei University", "Taipei")
```

上面這幾行程式碼中，定義了一個類別叫做School，而後定義了一個物件（實例instance）叫做ntpu，這個物件本身內部有兩個成員變數，**name** 和 **location**。
```python=
print(ntpu.name) # 取得值後直接將其印出來
ntpu.name = "台北大學" # 修改成員變數的值
```

### 成員變數是什麼
**成員變數（屬性）** 用來儲存物件狀態的變數，每個物件可以擁有自己的成員變數值，這些值定義了物件的特性或狀態。
```python=
class Car:
    def __init__(self, year, color):
        self.year = year
        self.color = color

toyota = Car(2020, "red")
honda = Car(2021, "blue")
```
像上面的程式碼中，定義了兩個不同的物件，來自於同個類別（Car），而他們透過建構式設定了不同的屬性。
**toyota**的年份是2020，顏色為red。
**honda**的年份是2021，顏色為blue。

### 成員函式是什麼
**成員函式（方法）** 是類別內部定義的函式，通常用來操作物件的成員變數或執行與物件相關的邏輯。
```python=
class Animal:
    def __init__(self, postion, speed):
        self.position = postion
        self.speed = speed

    def forward(self):
        self.position += self.speed

people = Animal(0, 10)
turtle = Animal(0, 1)

people.forward()
turtle.forward()

print(people.position)
# 10
print(turtle.position)
# 1
```
在上面的程式碼當中，定義了兩個不同的動物，一個是people另一個是turtle。
當中烏龜的移動速度較慢所以我們定義他的速度為1，而人則為10，兩個的初始位置皆為0。
同時調用他們兩個的forward函式，讓他們往前走一步，最後印出來的結果是他們的位置都改變了，並且還會根據當初定義的速度有不同距離的變化。
### 建構式是什麼
**建構式（Constructor）** 是類別中的一種特殊方法，用來在創建物件時初始化該物件的狀態。
```python=
class Animal:
    def __init__(self, postion, speed):
        self.position = postion
        self.speed = speed
```
例如上面的程式碼中的建構式告訴我們如果要建立一個Animal類別的物件出來的話，第一個參數為position，第二個為speed，而成員變數在python中是定義在建構式內部的。
### self是什麼
**self**在python裡面是一個約定成俗的參數名字，用來代表當前物件的參數名稱。
### 為什麼成員函式前面都要加self
因為python在呼叫成員函式時，會將物件自己本身當作第一個參數傳入函式，所以我們需要有一個參數用來代表進行函式時這個物件本身，這個參數名字就是self。

以下例子為python呼叫成員函式時會做的事
```python=
people.forward()
# python會將其變成Animal.forward(people)然後進行呼叫。
```
```python=
# 進來函式後
def forward(self):
# 在這裡people傳進來變成了參數self，因此我們可以透過self.speed調用他的成員變數
# 就像是之前的people.speed一樣。
```
實際上可以不用取名叫做self，如果你想要你可以定義成
```python=
def forward(我自己):
# 這樣就可以透過 我自己.speed調用他自己的成員變數。
```
但是python整體社群的慣例將其命名為self，所以盡量還是命名為self。

### python官方範例
Every variable in Python is an object
在python程式語言當中，所有定義的變數實際上都是object，每一個都是一個類別的instance。
例如my_list = []，my_list實際上是的型態是List，而他可以呼叫的方法有my_list.clear()等等。

## 第二章 物件導向三大特性
### 繼承
繼承允許一個類別（子類別）從另一個類別（父類別）中繼承屬性和方法。透過繼承，子類別可以重用父類別的代碼，並在此基礎上進行擴展或修改（override）。範例如下
```python=
class Entity:
    def __init__(self, health, element):
        self.health = health
        self.element = element
    
    def attack(self, enemy):
        pass

class Warrior(Entity):
    def __init__(self, health, element, strength):
        super().__init__(health, element)
        self.strength = strength
    
    def attack(self, enemy):
        damage = self.strength

        self.strength -= 1
        if self.element == "fire" and enemy.element == "ice":
            damage *= 2
        elif self.element == "ice" and enemy.element == "fire":
            damage /= 2
        enemy.health -= damage
        print(f"Warrior attacks! Enemy health: {enemy.health}")

class Wizard(Entity):
    def __init__(self, health, element, magic_power):
        super().__init__(health, element)
        self.magic_power = magic_power
    
    def attack(self, enemy):
        damage = self.magic_power
        self.health += 1

        if self.element == "fire" and enemy.element == "ice":
            damage *= 2
        elif self.element == "ice" and enemy.element == "fire":
            damage /= 2
        enemy.health -= damage
        print(f"Wizard attacks! Enemy health: {enemy.health}")
        
        
billy = Warrior(100, "fire", 10)
# Warrior attacks! Enemy health: 80
bob = Wizard(100, "ice", 10)
# Wizard attacks! Enemy health: 95.0


billy.attack(bob)
bob.attack(billy)

list_of_entities = [billy, bob]
for entity in list_of_entities:
    print(f"Entity health: {entity.health}, Element: {entity.element}")
# Entity health: 95.0, Element: fire
# Entity health: 81, Element: ice
```
在上面的程式碼當中，我們先定義了一個父類別（Entity），而後我們定義了兩個子類別（Warrior and Wizard）各自都繼承Entity，父類別當中，定義了大家共有的屬性和方法。
兩個子類別則自己新增了屬於自己類別的屬性（strength and magic_power），以及修改（覆蓋掉）父類別有的函式。
![image](https://hackmd.io/_uploads/SkdLcRrgex.png)
由程式碼可看出繼承的其中一個優點：共同的變數或方法可以不用重複寫，像是父類別中的health和element，在子類別繼承以後，就不需要重新定義這兩個變數，但他們會需要呼叫super().__init__()也就是父類別的建構子，而之後子類別一樣可以透過instance.element來存取成員變數。
可以想像成父類別是一個房子的骨架，而子類別是房子後續的搭建，這些後續的搭建都需要在父類別的基礎上繼續搭建，所以必須先建構父類別，因此才會在子類別的建構子中呼叫父類別的建構子。
而super關鍵字
```python=
def __init__(self, health, element, strength): # 子類別的建構子
        super().__init__(health, element) # 必須呼叫父類別的建構子
        self.strength = strength
```

上面的attack()函式在父類別當中被定義過，而子類別可以選擇要不要自己重新撰寫一次函式覆蓋掉父類別的函式，或者是也可以不寫，沿用父類別的函式。
```python=
class Father:
    def __init__(self, name, age):
        self.name = name
        self.age = age

    def get_name(self):
        return f"{self.name} call from Father"

class Son(Father):
    def __init__(self, name, age):
        super().__init__(name, age)

son1 = Son("John", 25)
print(son1.get_name()) # Output: John call from Father
```
像上面的程式碼中Son類別並沒有額外覆蓋父類別的函式，則後面在呼叫這函式時會使用到父類別定義的函式。

而下面的程式碼則覆蓋掉父類別的函式，因此輸出變成Son所定義的字串
```python=
class Father:
    def __init__(self, name, age):
        self.name = name
        self.age = age

    def get_name(self):
        return f"{self.name} call from Father"

class Son(Father):
    def __init__(self, name, age):
        super().__init__(name, age)
    def get_name(self):
        return f"{self.name} call from Son"

son1 = Son("John", 25)
print(son1.get_name()) # Output: John call from Son
```
而繼承的優點除了可以減少重複的程式碼，最重要的是讓程式碼更容易維護，因為在實務上像是遊戲中的角色系統，可能會像warrior及wizard那樣同時繼承entity，這樣在後續的維護上會更加方便，例如每個物體都想要新增一個屬性：耐力值，那麼就只需要在entity類別裡面再加上一個屬性即可。

### 多型
```python=
class Entity:
    def __init__(self, health, element):
        self.health = health
        self.element = element
    
    def attack(self, enemy):
        pass

class Warrior(Entity):
    def __init__(self, health, element, strength):
        super().__init__(health, element)
        self.strength = strength
    
    def attack(self, enemy):
        damage = self.strength

        self.strength -= 1
        if self.element == "fire" and enemy.element == "ice":
            damage *= 2
        elif self.element == "ice" and enemy.element == "fire":
            damage /= 2
        enemy.health -= damage
        print(f"Warrior attacks! Enemy health: {enemy.health}")

class Wizard(Entity):
    def __init__(self, health, element, magic_power):
        super().__init__(health, element)
        self.magic_power = magic_power
    
    def attack(self, enemy):
        damage = self.magic_power
        self.health += 1

        if self.element == "fire" and enemy.element == "ice":
            damage *= 2
        elif self.element == "ice" and enemy.element == "fire":
            damage /= 2
        enemy.health -= damage
        print(f"Wizard attacks! Enemy health: {enemy.health}")
        
        
billy = Warrior(100, "fire", 10)
# Warrior attacks! Enemy health: 80
bob = Wizard(100, "ice", 10)
# Wizard attacks! Enemy health: 95.0

billy.attack(bob)
bob.attack(billy)

list_of_entities = [billy, bob]
for entity in list_of_entities:
    print(f"Entity health: {entity.health}, Element: {entity.element}")
# Entity health: 95.0, Element: fire
# Entity health: 81, Element: ice
```
同樣是上面的例子，同樣都是呼叫 attack 這個函式，但是因為billy 和 bob 的類別不同且各自都有覆蓋掉父類別的函式，所以有不同的表現，這就是多型，多型允許不同類別以統一的方式進行調用，但實際執行的行為會因為物件的具體類型有所不同。

### 封装
在設計類別時，將某些部分隱藏起來，使外界要使用這個類別時，只能使用類別定義的公開函式及變數。
#### 這樣有什麼優點
隱藏實作細節：像是python官方的資料型態 list 內部實作的細節使用者無法知道，但使用者可以知道的是呼叫clear()可以清空現在list內的資料。這種隱藏實作細節（抽象），讓使用者不需要知道細節的方式在現今程式設計當中很常見。
```python=
def shuffle(self, x): 
        """Shuffle list x in place, and return None."""

        randbelow = self._randbelow
        for i in reversed(range(1, len(x))):
            # pick an element in x[:i+1] with which to exchange x[i]
            j = randbelow(i + 1)
            x[i], x[j] = x[j], x[i]
```
上面的程式碼是官方原始碼的random.py中的shuffle，可以讓一個list中的元素進行打亂，在這個例子當中shuffle函式使用到了_randbelow（實際上這個變數是一個函式變數，來自於_randbelow_with_getrandbits），避免外界能夠直接呼叫到這些私有函式。


## 進階
### 鴨子型別
也就是說不管類型的型別為何，只要實作到這個同名的成員函式，就可以傳入這個函式。
```python=
class Matrix:
    def __init__(self, rows, cols):
        self.rows = rows
        self.cols = cols
        self.data = [[0] * cols for _ in range(rows)]

    def print(self):
        for row in self.data:
            print(" ".join(map(str, row)))


class Player:
    def __init__(self, name):
        self.name = name
        self.score = 0
    
    def print(self):
        print(f"Player: {self.name}, Score: {self.score}")

def print_object(obj):
    if hasattr(obj, 'print'):
        obj.print()
    else:
        print("錯誤: 不是鴨子型別")

obj_list = [Matrix(3, 3), Player("Alice"), "這不是鴨子型別"]

for obj in obj_list:
    print_object(obj)
# 輸出：
# 0 0 0
# 0 0 0
# 0 0 0
# Player: Alice, Score: 0
# 錯誤: 不是鴨子型別
```
這段程式碼與多型的不同之處在於沒有使用繼承，但卻可以實現差不多的效果。
與多型的使用場景不同，多型的每個類別都繼承一個 base class ，他們之間都有一些關係存在。
而在上面鴨子型別的例子當中，Matrix與Player一點關係都沒有，在實務中如果同時需要很多多型的需求，使用繼承的方式會導致程式碼冗長。
例：可以print的定義一個printable類別，可以被攻擊的定義一個attackable類別，然後想要讓一個類別可以被print就繼承printable，同樣的操作重複很多，這樣一個類別就會繼承好幾個不同類別，這樣的程式碼會非常的長，改為使用鴨子型別的話，每個類別不用特別繼承多種類別，整體的程式碼也會更簡潔。



```markdown=
A. HISTORY OF THE SOFTWARE
==========================

Python was created in the early 1990s by Guido van Rossum at Stichting
Mathematisch Centrum (CWI, see https://www.cwi.nl) in the Netherlands
as a successor of a language called ABC.  Guido remains Python's
principal author, although it includes many contributions from others.

In 1995, Guido continued his work on Python at the Corporation for
National Research Initiatives (CNRI, see https://www.cnri.reston.va.us)
in Reston, Virginia where he released several versions of the
software.

In May 2000, Guido and the Python core development team moved to
BeOpen.com to form the BeOpen PythonLabs team.  In October of the same
year, the PythonLabs team moved to Digital Creations, which became
Zope Corporation.  In 2001, the Python Software Foundation (PSF, see
https://www.python.org/psf/) was formed, a non-profit organization
created specifically to own Python-related Intellectual Property.
Zope Corporation was a sponsoring member of the PSF.

All Python releases are Open Source (see https://opensource.org for
the Open Source Definition).  Historically, most, but not all, Python
releases have also been GPL-compatible; the table below summarizes
the various releases.

    Release         Derived     Year        Owner       GPL-
                    from                                compatible? (1)

    0.9.0 thru 1.2              1991-1995   CWI         yes
    1.3 thru 1.5.2  1.2         1995-1999   CNRI        yes
    1.6             1.5.2       2000        CNRI        no
    2.0             1.6         2000        BeOpen.com  no
    1.6.1           1.6         2001        CNRI        yes (2)
    2.1             2.0+1.6.1   2001        PSF         no
    2.0.1           2.0+1.6.1   2001        PSF         yes
    2.1.1           2.1+2.0.1   2001        PSF         yes
    2.1.2           2.1.1       2002        PSF         yes
    2.1.3           2.1.2       2002        PSF         yes
    2.2 and above   2.1.1       2001-now    PSF         yes

Footnotes:

(1) GPL-compatible doesn't mean that we're distributing Python under
    the GPL.  All Python licenses, unlike the GPL, let you distribute
    a modified version without making your changes open source.  The
    GPL-compatible licenses make it possible to combine Python with
    other software that is released under the GPL; the others don't.

(2) According to Richard Stallman, 1.6.1 is not GPL-compatible,
    because its license has a choice of law clause.  According to
    CNRI, however, Stallman's lawyer has told CNRI's lawyer that 1.6.1
    is "not incompatible" with the GPL.

Thanks to the many outside volunteers who have worked under Guido's
direction to make these releases possible.


B. TERMS AND CONDITIONS FOR ACCESSING OR OTHERWISE USING PYTHON
===============================================================

Python software and documentation are licensed under the
Python Software Foundation License Version 2.

Starting with Python 3.8.6, examples, recipes, and other code in
the documentation are dual licensed under the PSF License Version 2
and the Zero-Clause BSD license.

Some software incorporated into Python is under different licenses.
The licenses are listed with code falling under that license.


PYTHON SOFTWARE FOUNDATION LICENSE VERSION 2
--------------------------------------------

1. This LICENSE AGREEMENT is between the Python Software Foundation
("PSF"), and the Individual or Organization ("Licensee") accessing and
otherwise using this software ("Python") in source or binary form and
its associated documentation.

2. Subject to the terms and conditions of this License Agreement, PSF hereby
grants Licensee a nonexclusive, royalty-free, world-wide license to reproduce,
analyze, test, perform and/or display publicly, prepare derivative works,
distribute, and otherwise use Python alone or in any derivative version,
provided, however, that PSF's License Agreement and PSF's notice of copyright,
i.e., "Copyright (c) 2001 Python Software Foundation; All Rights Reserved"
are retained in Python alone or in any derivative version prepared by Licensee.

3. In the event Licensee prepares a derivative work that is based on
or incorporates Python or any part thereof, and wants to make
the derivative work available to others as provided herein, then
Licensee hereby agrees to include in any such work a brief summary of
the changes made to Python.

4. PSF is making Python available to Licensee on an "AS IS"
basis.  PSF MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PSF MAKES NO AND
DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON WILL NOT
INFRINGE ANY THIRD PARTY RIGHTS.

5. PSF SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON,
OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.

6. This License Agreement will automatically terminate upon a material
breach of its terms and conditions.

7. Nothing in this License Agreement shall be deemed to create any
relationship of agency, partnership, or joint venture between PSF and
Licensee.  This License Agreement does not grant permission to use PSF
trademarks or trade name in a trademark sense to endorse or promote
products or services of Licensee, or any third party.

8. By copying, installing or otherwise using Python, Licensee
agrees to be bound by the terms and conditions of this License
Agreement.


BEOPEN.COM LICENSE AGREEMENT FOR PYTHON 2.0
-------------------------------------------

BEOPEN PYTHON OPEN SOURCE LICENSE AGREEMENT VERSION 1

1. This LICENSE AGREEMENT is between BeOpen.com ("BeOpen"), having an
office at 160 Saratoga Avenue, Santa Clara, CA 95051, and the
Individual or Organization ("Licensee") accessing and otherwise using
this software in source or binary form and its associated
documentation ("the Software").

2. Subject to the terms and conditions of this BeOpen Python License
Agreement, BeOpen hereby grants Licensee a non-exclusive,
royalty-free, world-wide license to reproduce, analyze, test, perform
and/or display publicly, prepare derivative works, distribute, and
otherwise use the Software alone or in any derivative version,
provided, however, that the BeOpen Python License is retained in the
Software, alone or in any derivative version prepared by Licensee.

3. BeOpen is making the Software available to Licensee on an "AS IS"
basis.  BEOPEN MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, BEOPEN MAKES NO AND
DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE SOFTWARE WILL NOT
INFRINGE ANY THIRD PARTY RIGHTS.

4. BEOPEN SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF THE
SOFTWARE FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS
AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THE SOFTWARE, OR ANY
DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.

5. This License Agreement will automatically terminate upon a material
breach of its terms and conditions.

6. This License Agreement shall be governed by and interpreted in all
respects by the law of the State of California, excluding conflict of
law provisions.  Nothing in this License Agreement shall be deemed to
create any relationship of agency, partnership, or joint venture
between BeOpen and Licensee.  This License Agreement does not grant
permission to use BeOpen trademarks or trade names in a trademark
sense to endorse or promote products or services of Licensee, or any
third party.  As an exception, the "BeOpen Python" logos available at
http://www.pythonlabs.com/logos.html may be used according to the
permissions granted on that web page.

7. By copying, installing or otherwise using the software, Licensee
agrees to be bound by the terms and conditions of this License
Agreement.


CNRI LICENSE AGREEMENT FOR PYTHON 1.6.1
---------------------------------------

1. This LICENSE AGREEMENT is between the Corporation for National
Research Initiatives, having an office at 1895 Preston White Drive,
Reston, VA 20191 ("CNRI"), and the Individual or Organization
("Licensee") accessing and otherwise using Python 1.6.1 software in
source or binary form and its associated documentation.

2. Subject to the terms and conditions of this License Agreement, CNRI
hereby grants Licensee a nonexclusive, royalty-free, world-wide
license to reproduce, analyze, test, perform and/or display publicly,
prepare derivative works, distribute, and otherwise use Python 1.6.1
alone or in any derivative version, provided, however, that CNRI's
License Agreement and CNRI's notice of copyright, i.e., "Copyright (c)
1995-2001 Corporation for National Research Initiatives; All Rights
Reserved" are retained in Python 1.6.1 alone or in any derivative
version prepared by Licensee.  Alternately, in lieu of CNRI's License
Agreement, Licensee may substitute the following text (omitting the
quotes): "Python 1.6.1 is made available subject to the terms and
conditions in CNRI's License Agreement.  This Agreement together with
Python 1.6.1 may be located on the internet using the following
unique, persistent identifier (known as a handle): 1895.22/1013.  This
Agreement may also be obtained from a proxy server on the internet
using the following URL: http://hdl.handle.net/1895.22/1013".

3. In the event Licensee prepares a derivative work that is based on
or incorporates Python 1.6.1 or any part thereof, and wants to make
the derivative work available to others as provided herein, then
Licensee hereby agrees to include in any such work a brief summary of
the changes made to Python 1.6.1.

4. CNRI is making Python 1.6.1 available to Licensee on an "AS IS"
basis.  CNRI MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, CNRI MAKES NO AND
DISCLAIMS ANY REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS
FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF PYTHON 1.6.1 WILL NOT
INFRINGE ANY THIRD PARTY RIGHTS.

5. CNRI SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PYTHON
1.6.1 FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS
A RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PYTHON 1.6.1,
OR ANY DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.

6. This License Agreement will automatically terminate upon a material
breach of its terms and conditions.

7. This License Agreement shall be governed by the federal
intellectual property law of the United States, including without
limitation the federal copyright law, and, to the extent such
U.S. federal law does not apply, by the law of the Commonwealth of
Virginia, excluding Virginia's conflict of law provisions.
Notwithstanding the foregoing, with regard to derivative works based
on Python 1.6.1 that incorporate non-separable material that was
previously distributed under the GNU General Public License (GPL), the
law of the Commonwealth of Virginia shall govern this License
Agreement only as to issues arising under or with respect to
Paragraphs 4, 5, and 7 of this License Agreement.  Nothing in this
License Agreement shall be deemed to create any relationship of
agency, partnership, or joint venture between CNRI and Licensee.  This
License Agreement does not grant permission to use CNRI trademarks or
trade name in a trademark sense to endorse or promote products or
services of Licensee, or any third party.

8. By clicking on the "ACCEPT" button where indicated, or by copying,
installing or otherwise using Python 1.6.1, Licensee agrees to be
bound by the terms and conditions of this License Agreement.

        ACCEPT


CWI LICENSE AGREEMENT FOR PYTHON 0.9.0 THROUGH 1.2
--------------------------------------------------

Copyright (c) 1991 - 1995, Stichting Mathematisch Centrum Amsterdam,
The Netherlands.  All rights reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

ZERO-CLAUSE BSD LICENSE FOR CODE IN THE PYTHON DOCUMENTATION
----------------------------------------------------------------------

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
```)";
    auto huffman = HuffmanCoding<decltype(data)>();
    huffman.buildTree(data);

    auto encoded = huffman.encode();

    // get encoded size
    size_t size = 0;
    for (auto& element : data) {
        size += encoded[element].size();
    }

    std::println("encoded size : {}", size);
    std::println("data size : {}", data.size() * 8);
    std::println("compression ratio : {}", (float)size / (data.size() * 8));

    std::println("======================================");
    // ls77 compress
    auto encodedLs77 = ls77Encode(data);
    std::println("ls77 encoded size : {}", encodedLs77.size());
    std::println("ls77 data size : {}", data.size());
    std::println("ls77 compression ratio : {}",
                 (float)encodedLs77.size() / data.size());

#ifdef WIN32
    f9ay::test::windows::Windows windows{};
    std::visit(
        [&windows]<typename T>(T&& arg) {
            using T0 = std::decay_t<T>;
            windows.show(arg);
        },
        result);
#endif
}