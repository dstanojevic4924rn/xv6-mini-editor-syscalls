#!/bin/bash

FAJL1="testerkernel.c"
FAJL2="testeruser.c"
FAJL3="tester.c"

GLAVNI_FOLDER="novi"



if [[ ! -f "$FAJL1" || ! -f "$FAJL2" || ! -f "$FAJL3" ]]; then
    echo "Greska: Fajlovi za kopiranje ($FAJL1 ili $FAJL2 ili $FAJL3) ne postoje u trenutnom folderu!"
    exit 1
fi


for student_dir in "$GLAVNI_FOLDER"/*/; do
    ime_studenta=$(basename "$student_dir")

    echo "------------------------------------------------"
    echo "Obradjujem studenta: $ime_studenta"

    PARAM_H=$(find "$student_dir" -type f -name "param.h" | head -n 1)
    MAKEFILE=$(find "$student_dir" -type f -name "Makefile" | head -n 1)
    USER_DIR=$(find "$student_dir" -type d -name "user" | head -n 1)

    if [[ -n "$PARAM_H" && -f "$PARAM_H" ]]; then
        sed -i 's/^#define FSSIZE.*/#define FSSIZE       10000/' "$PARAM_H"
        echo " [+] FSSIZE uspesno postavljen na 10000."
    else
        echo " [-] Greska: Nije pronadjen param.h!"
    fi


    if [[ -n "$MAKEFILE" && -f "$MAKEFILE" ]]; then

        sed -i 's/UPROGS=\\/UPROGS=\\\n\t$U\/_testerkernel\\\n\t$U\/_testeruser\\\n\t$U\/_tester\\/' "$MAKEFILE"

	sed -i 's/UPROGS= \\/UPROGS= \\\n\t$U\/_testerkernel\\\n\t$U\/_testeruser\\\n\t$U\/_tester\\/' "$MAKEFILE"

        echo " [+] Dodati _tester, _testerkernel i _testeruser u Makefile."
    else
        echo " [-] Greska: Nije pronadjen Makefile!"
    fi


    if [[ -n "$USER_DIR" && -d "$USER_DIR" ]]; then
        cp "$FAJL1" "$USER_DIR/"
        cp "$FAJL2" "$USER_DIR/"
        cp "$FAJL3" "$USER_DIR/"
        echo " [+] Testeri prebaceni u 'user' direktorijum."
    else
        echo " [-] Greska: Nije pronadjen 'user' direktorijum!"
    fi

done

echo "------------------------------------------------"
echo "SVE ZAVRSENO!"
