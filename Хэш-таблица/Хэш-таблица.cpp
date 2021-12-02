#include <iostream>
#include <string>
#define ANY_KEY "\nНажмите любую клавишу для выхода из программы...\n"

int HashFunctionHorner(const std::string& s, int table_size, const int key)
{
    int hash_result = 0;
    for (int i = 0; s[i] != s[s.size()]; ++i)
        hash_result = (key * hash_result + s[i]) % table_size;
    hash_result = (hash_result * 2 + 1) % table_size;

    return hash_result;
}

struct HashFunction1
{
    int operator()(const std::string& s, int table_size) const
    {
        return HashFunctionHorner(s, table_size, table_size - 1); // ключи должны быть взаимопросты, используем числа <размер таблицы> плюс и минус один.
    }
};

struct HashFunction2
{
    int operator()(const std::string& s, int table_size) const
    {
        return HashFunctionHorner(s, table_size, table_size + 1);
    }
};



template <typename T, class THash1 = HashFunction1, class THash2 = HashFunction2>
class HashTable
{
    static const int default_size = 8; // начальный размер нашей таблицы
    constexpr static const double rehash_size = 0.75; // коэффициент, при котором произойдет увеличение таблицы
    
    struct Node
    {
        T value;
        bool state; // если значение флага state = false, значит элемент массива был удален (deleted)
        Node(const T& value_) : value(value_), state(true) {}
    };

    Node** arr; // соответственно в массиве будут храниться структуры Node*
    int size; // сколько элементов у нас сейчас в массиве (без учета deleted)
    int buffer_size; // размер самого массива, сколько памяти выделено под хранение нашей таблицы
    int size_all_non_nullptr; // сколько элементов у нас сейчас в массиве (с учетом deleted)

    private:

       

        void Resize()
        {
            int past_buffer_size = buffer_size;
            buffer_size *= 2;
            size_all_non_nullptr = 0;
            size = 0;
            Node** arr2 = new Node * [buffer_size];
            for (int i = 0; i < buffer_size; ++i)
                arr2[i] = nullptr;
            std::swap(arr, arr2);
            for (int i = 0; i < past_buffer_size; ++i)
            {
                if (arr2[i] && arr2[i]->state)
                    Add(arr2[i]->value); // добавляем элементы в новый массив
            }
            // удаление предыдущего массива
            for (int i = 0; i < past_buffer_size; ++i)
                if (arr2[i])
                    delete arr2[i];
            delete[] arr2;
        }

    public:

        HashTable()
        {
            buffer_size = default_size;
            size = 0;
            size_all_non_nullptr = 0;
            arr = new Node * [buffer_size];
            // заполняем nullptr - то есть если значение отсутствует
            //и никто раньше по этому адресу не обращался
            for (int i = 0; i < buffer_size; ++i)
                arr[i] = nullptr; 
        }

        ~HashTable()
        {
            for (int i = 0; i < buffer_size; ++i)
                if (arr[i])
                    delete arr[i];

            delete[] arr;
        }

        bool Remove(const T& value, const THash1& hash1 = THash1(), const THash2& hash2 = THash2())
        {
            int h1 = hash1(value, buffer_size);
            int h2 = hash2(value, buffer_size);
            int i = 0;

            while (arr[h1] != nullptr && i < buffer_size)
            {
                if (arr[h1]->value == value && arr[h1]->state)
                {
                    arr[h1]->state = false;
                    --size;
                    return true;
                }
                h1 = (h1 + h2) % buffer_size;
                ++i;
            }

            return false;
        }

        void Rehash()
        {
            size_all_non_nullptr = 0;
            size = 0;

            Node** arr2 = new Node* [buffer_size];

            for (int i = 0; i < buffer_size; ++i)
                arr2[i] = nullptr;

            std::swap(arr, arr2);

            for (int i = 0; i < buffer_size; ++i)
            {
                if (arr2[i] && arr2[i]->state)
                    Add(arr2[i]->value);
            }

            // удаление предыдущего массива
            for (int i = 0; i < buffer_size; ++i)
                if (arr2[i])
                    delete arr2[i];

            delete[] arr2;
        }

        void Print()
        {
            std::cout << "Кол-во элементов в таблице, учитывая удалённые: " << size_all_non_nullptr << '\n';
            std::cout << "Не учитывая: " << size << '\n';
            std::cout << "Элементы:\n";

            for (int i = 0; i < buffer_size; i++)
            {
                if (arr[i] && arr[i]->state)
                    std::cout << arr[i]->value << '\n';
            }
        }

        bool Find(const T& value, const THash1& hash1 = THash1(), const THash2& hash2 = THash2())
        {
            int h1 = hash1(value, buffer_size); // значение, отвечающее за начальную позицию
            int h2 = hash2(value, buffer_size); // значение, ответственное за "шаг" по таблице
            int i = 0;

            while (arr[h1] != nullptr && i < buffer_size)
            {
                if (arr[h1]->value == value && arr[h1]->state)
                    return true; // такой элемент есть
                h1 = (h1 + h2) % buffer_size;
                ++i; // если у нас i >=  buffer_size, значит мы уже обошли абсолютно все ячейки, именно для этого мы считаем i, иначе мы могли бы зациклиться.
            }
            return false;
        }

       


        bool Add(T& value, const THash1& hash1 = THash1(), const THash2& hash2 = THash2())
        {
            if (size + 1 > int(rehash_size * buffer_size))
                Resize();
            else if (size_all_non_nullptr > 2 * size)
                Rehash(); // происходит рехеш, так как слишком много deleted-элементов
            int h1 = hash1(value, buffer_size);
            int h2 = hash2(value, buffer_size);
            int i = 0;
            int first_deleted = -1; // запоминаем первый подходящий (удаленный) элемент
            while (arr[h1] != nullptr && i < buffer_size)
            {
                if (arr[h1]->value == value && arr[h1]->state)
                    return false; // такой элемент уже есть, а значит его нельзя вставлять повторно

                if (!arr[h1]->state && first_deleted == -1) // находим место для нового элемента
                    first_deleted = h1;

                h1 = (h1 + h2) % buffer_size;
                ++i;
            }

            if (first_deleted == -1) // если не нашлось подходящего места, создаем новый Node
            {
                arr[h1] = new Node(value);
                ++size_all_non_nullptr; // так как мы заполнили один пробел, не забываем записать, что это место теперь занято
            }
            else
            {
                arr[first_deleted]->value = value;
                arr[first_deleted]->state = true;
            }

            ++size; // и в любом случае мы увеличили количество элементов
            return true;
        }
};

void printTask()
{
    std::cout << "Данная программа предназначена для работы с хэш-таблицей:\n";

    std::cout << "Варианты действий:\n";
    std::cout << "1 - добавить элемент\n";
    std::cout << "2 - удалить элемент\n";
    std::cout << "3 - найти элемент\n";
    std::cout << "4 - вывести инфу\n";
    std::cout << "5 - выход из программы\n\n\n";
}

std::string getUserInput()
{
    const int MIN_SIZE = 0;
    std::string input;

    bool isIncorrect;

    do {
        isIncorrect = false;

        std::cout << "Введите строку:";

        std::cin >> input;

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (input.size() == MIN_SIZE) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            isIncorrect = true;
            std::cout << "Введите не пустую строку!";
        }
    } while (isIncorrect);

    return input;
}

int getChoice()
{
    int choice;
    bool isIncorrect;

    do {
        isIncorrect = false;

        std::cout << "Введите Ваш выбор:";
        
        std::cin >> choice;

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice < 1 || choice > 5) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            isIncorrect = true;
            std::cout << "Введите число от 1 до 5!";
        }
    } while (isIncorrect);

    return choice;
}

void processChoice(HashTable<std::string, HashFunction1, HashFunction2>& hash)
{
    unsigned int start_time, end_time, search_time;
    int choice;
    std::string input;


    printTask();
    choice = getChoice();

    switch (choice)
    {
    case 1:
        input = getUserInput();

        if (hash.Add(input))
            std::cout << "\nЭлемент был успешно добавлен\n";
        else
            std::cout << "\nТакой элемент уже есть\n";
        break;

    case 2:
        input = getUserInput();

        if(hash.Remove(input))
            std::cout << "\nЭлемент был успешно удалён\n";
        else
            std::cout << "\nЭлемент не был найден\n";
        break;
    case 3:
        input = getUserInput();

        start_time = clock(); // начальное время

        if (hash.Find(input))
            std::cout << "\nЭлемент был найден\n";
        else
            std::cout << "\nЭлемент не был найден\n";

        end_time = clock(); // конечное время
        search_time = end_time - start_time; // искомое время

        std::cout << "Время выполнения поиска:" << search_time << " мс\n";
        break;

    case 4:
        hash.Print();
        break;
    case 5:
        break;
    }

    if (choice != 5) {
        system("pause");
        system("cls");
        processChoice(hash);
    }
}

int main() {
    setlocale(LC_ALL, "RUS");

    HashFunction1 func1 = HashFunction1();
    HashFunction2 func2 = HashFunction2();
    HashTable<std::string, HashFunction1, HashFunction2> hash = HashTable<std::string, HashFunction1, HashFunction2>();

   
    processChoice(hash);

    std::cout << ANY_KEY;
    return EXIT_SUCCESS;
}