/*
 * Course   : 25ECE121 - Object Oriented Programming using C++ Lab
 * Task     : Task 2 (Weeks 7-13) - Student Result Management System
 * Name     : A Mahilan
 * USN      : NB25ECE001
 *
 * Concepts demonstrated:
 *   - Inheritance            : Result derives from Student
 *   - Runtime polymorphism   : virtual functions called through base-class pointers
 *   - Exception handling     : custom InvalidMarksException for marks outside 0-100
 *   - File handling          : records saved to / loaded from students.txt
 *   - Templates              : generic function template getHighest()
 *   - STL containers         : std::vector<std::unique_ptr<Student>>
 *   - STL algorithms/lambdas : copy_if, any_of, sort, max_element with lambdas
 */

#include <algorithm>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

const int NUM_SUBJECTS = 5;
const int MAX_MARKS = 100;
const int PASS_MARKS = 40;
const std::string FILE_NAME = "students.txt";
const std::string SUBJECTS[NUM_SUBJECTS] = {"Maths", "Physics", "Electronics", "OOP", "Signals"};

// ---------------------------------------------------------------------------
// Custom exception for invalid marks
// ---------------------------------------------------------------------------
class InvalidMarksException : public std::exception {
    std::string message;

public:
    InvalidMarksException(const std::string &subject, int marks) {
        message = "Invalid marks " + std::to_string(marks) + " in " + subject +
                  " (must be between 0 and " + std::to_string(MAX_MARKS) + ")";
    }
    const char *what() const noexcept override { return message.c_str(); }
};

// ---------------------------------------------------------------------------
// Base class: Student
// ---------------------------------------------------------------------------
class Student {
protected:
    std::string usn;
    std::string name;

public:
    Student(const std::string &usn, const std::string &name) : usn(usn), name(name) {}
    virtual ~Student() = default;

    const std::string &getUsn() const { return usn; }
    const std::string &getName() const { return name; }

    // Virtual functions - overridden in Result
    virtual void displayDetails() const {
        std::cout << "USN : " << usn << "\nName: " << name << "\n";
    }
    virtual void displayMarks() const { std::cout << "No marks available.\n"; }
    virtual int getTotal() const { return 0; }
    virtual std::string toRecord() const { return usn + "|" + name; }
};

// ---------------------------------------------------------------------------
// Derived class: Result
// ---------------------------------------------------------------------------
class Result : public Student {
    std::vector<int> marks;
    int total;

    static void validate(const std::vector<int> &m) {
        for (size_t i = 0; i < m.size(); ++i) {
            if (m[i] < 0 || m[i] > MAX_MARKS) {
                throw InvalidMarksException(SUBJECTS[i], m[i]);
            }
        }
    }

public:
    Result(const std::string &usn, const std::string &name, const std::vector<int> &m)
        : Student(usn, name), marks(m), total(0) {
        validate(marks); // throws InvalidMarksException on bad input
        for (int x : marks) total += x;
    }

    double getPercentage() const { return total * 100.0 / (NUM_SUBJECTS * MAX_MARKS); }

    bool hasPassed() const {
        return std::all_of(marks.begin(), marks.end(), [](int m) { return m >= PASS_MARKS; });
    }

    std::string getGrade() const {
        if (!hasPassed()) return "F";
        double p = getPercentage();
        if (p >= 90) return "S";
        if (p >= 80) return "A";
        if (p >= 70) return "B";
        if (p >= 60) return "C";
        if (p >= 50) return "D";
        return "E";
    }

    int getTotal() const override { return total; }

    void displayDetails() const override {
        Student::displayDetails();
        std::cout << "Total Marks: " << total << " / " << NUM_SUBJECTS * MAX_MARKS
                  << "   Percentage: " << std::fixed << std::setprecision(2) << getPercentage()
                  << "%\n";
    }

    void displayMarks() const override {
        for (int i = 0; i < NUM_SUBJECTS; ++i) {
            std::cout << "  " << std::left << std::setw(12) << SUBJECTS[i] << ": " << marks[i]
                      << "\n";
        }
        std::cout << "Result: " << (hasPassed() ? "PASS" : "FAIL") << "   Grade: " << getGrade()
                  << "\n";
    }

    std::string toRecord() const override {
        std::string rec = Student::toRecord();
        for (int m : marks) rec += "|" + std::to_string(m);
        return rec;
    }
};

// ---------------------------------------------------------------------------
// Function template
// ---------------------------------------------------------------------------
template <typename T>
T getHighest(const std::vector<T> &v) {
    return *std::max_element(v.begin(), v.end());
}

// ---------------------------------------------------------------------------
// File handling
// ---------------------------------------------------------------------------
using StudentList = std::vector<std::unique_ptr<Student>>;

bool saveToFile(const StudentList &students) {
    std::ofstream out(FILE_NAME);
    if (!out) return false;
    for (const auto &s : students) out << s->toRecord() << "\n";
    return true;
}

int loadFromFile(StudentList &students) {
    std::ifstream in(FILE_NAME);
    if (!in) throw std::runtime_error("Could not open " + FILE_NAME + " for reading");

    students.clear();
    std::string line;
    int skipped = 0;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string usn, name, field;
        std::vector<int> marks;
        std::getline(ss, usn, '|');
        std::getline(ss, name, '|');
        try {
            while (std::getline(ss, field, '|')) marks.push_back(std::stoi(field));
            if (marks.size() != NUM_SUBJECTS) throw std::runtime_error("wrong field count");
            students.push_back(std::make_unique<Result>(usn, name, marks));
        } catch (const std::exception &e) {
            ++skipped; // skip corrupt record
        }
    }
    return skipped;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void displayAll(const StudentList &students) {
    if (students.empty()) {
        std::cout << "No records to display.\n";
        return;
    }
    std::cout << "\n===== STUDENT RESULTS (" << students.size() << " records) =====\n";
    for (const auto &s : students) { // runtime polymorphism via base pointer
        std::cout << "----------------------------------------\n";
        s->displayDetails();
        s->displayMarks();
    }
    std::cout << "----------------------------------------\n";

    std::vector<int> totals;
    for (const auto &s : students) totals.push_back(s->getTotal());
    std::cout << "Highest total in class: " << getHighest(totals) << "\n";
}

void showAboveMark(const StudentList &students, int threshold) {
    std::vector<const Student *> above;
    std::for_each(students.begin(), students.end(), [&](const std::unique_ptr<Student> &s) {
        if (s->getTotal() > threshold) above.push_back(s.get());
    });

    std::sort(above.begin(), above.end(),
              [](const Student *a, const Student *b) { return a->getTotal() > b->getTotal(); });

    std::cout << "\nStudents scoring above " << threshold << " total marks: " << above.size()
              << "\n";
    for (const Student *s : above) {
        std::cout << "  " << std::left << std::setw(12) << s->getUsn() << std::setw(20)
                  << s->getName() << " Total: " << s->getTotal() << "\n";
    }
}

bool readInt(const std::string &text, int &value) {
    std::stringstream ss(text);
    return (ss >> value) && ss.eof();
}

void addStudent(StudentList &students) {
    std::string usn, name, line;
    std::cout << "Enter USN: ";
    std::getline(std::cin, usn);
    bool duplicate = std::any_of(students.begin(), students.end(),
                                 [&](const std::unique_ptr<Student> &s) { return s->getUsn() == usn; });
    if (duplicate) {
        std::cout << "Error: a student with USN " << usn << " already exists.\n";
        return;
    }
    std::cout << "Enter Name: ";
    std::getline(std::cin, name);

    std::vector<int> marks;
    try {
        for (int i = 0; i < NUM_SUBJECTS; ++i) {
            std::cout << "Enter marks in " << SUBJECTS[i] << " (0-" << MAX_MARKS << "): ";
            std::getline(std::cin, line);
            int m;
            if (!readInt(line, m)) throw std::invalid_argument("Marks must be a whole number");
            marks.push_back(m);
        }
        students.push_back(std::make_unique<Result>(usn, name, marks));
        std::cout << "Record added successfully.\n";
    } catch (const InvalidMarksException &e) {
        std::cout << "Exception caught: " << e.what() << "\nRecord NOT added.\n";
    } catch (const std::exception &e) {
        std::cout << "Exception caught: " << e.what() << "\nRecord NOT added.\n";
    }
}

int main() {
    StudentList students;
    std::string line;

    while (true) {
        std::cout << "\n===== Student Result Management System =====\n"
                  << "1. Add student\n"
                  << "2. Display all students\n"
                  << "3. Show students scoring above a total\n"
                  << "4. Save records to file\n"
                  << "5. Load records from file\n"
                  << "6. Exit\n"
                  << "Enter choice: ";
        if (!std::getline(std::cin, line)) break;

        int choice;
        if (!readInt(line, choice)) {
            std::cout << "Please enter a number from 1 to 6.\n";
            continue;
        }

        try {
            switch (choice) {
            case 1:
                addStudent(students);
                break;
            case 2:
                displayAll(students);
                break;
            case 3: {
                std::cout << "Enter minimum total marks: ";
                std::getline(std::cin, line);
                int threshold;
                if (!readInt(line, threshold)) {
                    std::cout << "Invalid number.\n";
                    break;
                }
                showAboveMark(students, threshold);
                break;
            }
            case 4:
                if (saveToFile(students))
                    std::cout << students.size() << " records saved to " << FILE_NAME << "\n";
                else
                    std::cout << "Error: could not write to " << FILE_NAME << "\n";
                break;
            case 5: {
                int skipped = loadFromFile(students);
                std::cout << students.size() << " records loaded from " << FILE_NAME;
                if (skipped) std::cout << " (" << skipped << " corrupt record(s) skipped)";
                std::cout << "\n";
                break;
            }
            case 6:
                std::cout << "Goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid choice. Try again.\n";
            }
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    return 0;
}
