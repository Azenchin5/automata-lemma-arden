
# 📘 Project: Recognition and Resolution of a Finite Automaton – Arden's Lemma

## 🎯 Project Objective

This project reads the description of a finite automaton from a structured text file, builds it in memory, verifies its coherence, and generates the regular expression it recognizes using **Arden’s Lemma**.

It combines **Flex**, **Bison**, and **C** to:
- Parse and analyze an automaton
- Verify state and transition validity
- Simulate words
- Resolve the automaton using Arden’s Lemma to deduce the recognized regular language

---

## 📂 Project Structure

| File           | Purpose                                  |
|----------------|-------------------------------------------|
| `automate.l`   | Lexical analysis using Flex               |
| `automate.y`   | Syntax analysis using Bison               |
| `main.c`       | Main program logic                        |
| `exemple.txt`  | Input file describing the automaton       |
| `mots.txt`     | List of words to test (optional)          |
| `automate.dot` | DOT format output for graph generation    |
| `Makefile`     | Compilation automation (Linux)            |

---

## 🧰 Features

✅ Structured parsing of automata from `exemple.txt`  
✅ Coherence verification (states, transitions, alphabet, etc.)  
✅ Automatic DOT graph generation (`automate.dot`)  
✅ Resolution of equations using **Arden's Lemma**  
✅ Interactive word simulation  
✅ Batch testing from `mots.txt`  
✅ Image generation via Graphviz (`automate.png`)

---

## ⚙️ Installation & Execution

### 🛠️ Compile (Linux)

Make sure you have the required tools:

```bash
sudo apt update
sudo apt install bison flex graphviz
make
```

### 🚀 Run

```bash
./automate
```

- Parses the automaton from `exemple.txt`
- Tests the words listed in `mots.txt` (if present)
- Prints whether each word is **ACCEPTED** or **REJECTED**
- Resolves the regular language using Arden's Lemma
- ✅ Automatically generates `automate.png` (Graphviz image)

---

## ✨ Sample Output

```txt
Starting analysis...
Section ETATS recognized.
Section ALPHABET recognized.
Section INITIALE recognized.
Section TRANSITIONS recognized.
Section FINAUX recognized.

Automaton structure:
- States: q0 q1
- Alphabet: a b
- Initial state: q0
- Transitions:
  q0 -> q1 : a
  q1 -> q0 : b
- Final states: q1

The automaton is coherent.
DOT file generated: automate.dot
Image created: automate.png

--- Final resolution (Arden's Lemma) ---
L_q0 = a.(b)*

Language accepted by the automaton: a.(b)*
```

---

## 👨‍💻 Authors

Project developed by **Aziz Gatti**  

