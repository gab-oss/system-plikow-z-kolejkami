#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BLOCK 256
#define SIZE 8192 /* 8 KB */
#define NR 32

struct inode{
    bool empty; /* czy opisuje pusty blok */
    char name; /*nazwa pliku */
    int f_block; /*nr pierwszego bloku */
    int size; /*rozmiar pliku*/
    int blocks; /*liczba zajmowanych blokow */
    int nr; /* ktory blok pliku opisuje*/
    int next; /* nastepny w liscie */
    int full; /* liczba zapisanych bajtow */
};

struct superblock{
    int f_inode;
   /* long tab; */
    int f_block;
  /*  size_t i_size; */
    int nr_of_files; /* liczba plikow */
 /*   int nr_of_blocks;  */
    int nr_of_free; /* liczba wolnych blokow */
};

int create(FILE * fp, struct superblock * s){
    fp = fopen("dysk", "wb+");
    struct inode tab[NR];
    int k;

    for(k = 0; k < NR; ++k){ /* wszystkie bloki oznaczone w tablicy jako puste */
            tab[k].empty = true;
            tab[k].name = '\0';
            tab[k].f_block = -1;
            tab[k].size = 0;
            tab[k].blocks = 0;
            tab[k].nr = -1;
            tab[k].next = -1;
            tab[k].full = 0;
    }


    if (fp == NULL)
    {
        perror("Nie udalo sie utworzyc dysku\n");
        return 1;
    }

    s->nr_of_files = 0; /* superblok dla pustego dysku */
    s->nr_of_free = NR;
    s->f_inode = sizeof(struct superblock);
    s->f_block = sizeof(struct superblock) + NR * sizeof(struct inode);

    fseek(fp, SIZE + sizeof(struct superblock) + NR * sizeof(struct inode) - 1, SEEK_SET);
    fputc('\0', fp); /* ustal rozmiar pliku */

    fseek(fp, 0L, SEEK_SET);
    fwrite(s, sizeof(struct superblock), 1, fp); /* zapisz superblok */
    fwrite(&tab, sizeof(struct inode), NR, fp); /* zapisz tablice inode*/

    fclose(fp);
    return 0;
}

int import_copy(FILE * fp, struct superblock * s, struct inode * tab){
    char iname; /* nazwa pliku do przekopiowania */
    int isize; /* rozmiar pliku do przekopiowania */
    FILE * ifp;
    char * file_buffer; /* do wczytania pliku */
    int count; /* ile potrzeba blokow */
    int size_to_write; /* ile bajtow jeszcze trzeba zapisac */
    int blsize; /*ile do zapisania w bloku */
    int i, j, k, l, first; /*l - licznik blokow pliku, first - index pierszego bloku */

    fp = fopen("dysk", "rb+");
    if (fp == NULL)
    {
        perror("Nie udalo sie otworzyc dysku\n");
        return 1;
    }

    printf("Nazwa pliku do skopiowania - nazwy jednoznakowe: ");
    scanf("%s", &iname);

    ifp = fopen(&iname, "rb");

    if (ifp == NULL)
    {
        perror("Nie udalo sie otworzyc pliku\n");
        return 1;
    }

    fread(s, sizeof(struct superblock), 1, fp); /* wczytaj superblok */
    fseek(fp, sizeof(struct superblock), SEEK_SET);
    fread(tab, sizeof(struct inode), NR, fp); /* wczytaj tablice inode */

    /*czy taki plik juz istnieje? */

     for(i = 0; i < NR; ++i){
        if(!tab[i].empty){
            if(tab[i].name == iname){
            printf("Plik juz istnieje\n");
            return 1;
            }
        }
     }

    /* czy wystarczy miejsca? */

    fseek(ifp, 0L, SEEK_END);
    isize = ftell(ifp);
    fseek(ifp, 0L, SEEK_SET);

    printf("%d\n", s->nr_of_free * BLOCK);
    printf("%d\n", isize);

    if(isize > s->nr_of_free * BLOCK || s->nr_of_files >= NR){
        perror("Za malo miejsca lub za duzo plikow na dysku\n");
            fclose(ifp);
            fclose(fp);
        return 1;
        }

    printf("Wystarczy miejsca\n");

    /* przekopiuj */
    file_buffer = (char *)malloc((isize + 1)*sizeof(char));
    fread(file_buffer, isize, 1, ifp);

    count = isize/BLOCK; /* oblicz ile trzeba blokow */
    if(count * BLOCK < isize)
        ++count;

    printf("Potrzeba %d blokow\n", count);
    s->nr_of_free = s->nr_of_free - count; /* -count blokow, +1 plik w superbloku */
    ++(s->nr_of_files);

    printf("%d plikow na dysku, %d wolnych blokow\n", s->nr_of_files, s->nr_of_free);
    first = -1;
    k = -1;
    l = 0;
    size_to_write = isize;

    printf("Plik %d bajtow\n", size_to_write);

    for(i = 0; i < NR; ++i){ /* szukaj wolnych blokow */
        if(tab[i].empty){

            printf("Wolny blok %d\n", i);

            if(size_to_write >= BLOCK){ /*ile skopiowac do bloku */
                blsize = BLOCK;
                size_to_write = size_to_write - BLOCK;
            }
            else{
                blsize = size_to_write;
                size_to_write = 0;
            }

            tab[i].full = blsize;
            printf("do zapisania %d bajtow\n", blsize);

            fseek(fp, s->f_block + i * BLOCK, SEEK_SET); /* znajdz poczatek bloku */
            fwrite(file_buffer + l * BLOCK, sizeof(char), blsize, fp); /*znajdz blok danych w pliku zewnetrznym */
            tab[i].blocks = count; /*zmien inode */
            tab[i].empty = false;

            tab[i].name = iname;

            tab[i].nr = l;

            printf("%d blok pliku\n", l);

            tab[i].size = isize;

            if (l == 0){ /* pierwszy blok pliku */
                 first = i;
                 printf("to pierwszy blok pliku\n");
            }
                else{
                    tab[k].next = i;
                    printf("%d poprzedni blok pliku\n", k);
                }

            if( l == count - 1 ){
                tab[i].next = -1;
                printf("to ostatni blok pliku\n");
                break;
            }


            tab[i].f_block = first;
            printf("%d to pierwszy blok pliku\n", first);

            k = i;
            ++l;
        }
    }

    fseek(fp, 0L, SEEK_SET);
    fwrite(s, sizeof(struct superblock), 1, fp); /*zapisz metadane */
    fwrite(tab, sizeof(struct inode), NR, fp);
    fclose(ifp);
    fclose(fp);
    return 0;
}

int export_copy(FILE * fp, struct superblock * s, struct inode * tab){
    char ename; /* nazwa pliku z kopia */
    int esize; /* rozmiar pliku z kopia */
    FILE * efp;
    char buffer[BLOCK]; /* do wczytania bloku */
    int count; /* ile plik ma blokow */
    int i, j, k;

    fp = fopen("dysk", "rb+");
    if (fp == NULL)
    {
        perror("Nie udalo sie otworzyc dysku\n");
        return 1;
    }

    printf("Nazwa pliku do skopiowania - nazwy jednoznakowe: ");
    scanf("%s", &ename);

    efp = fopen(&ename, "wb+");

    if (efp == NULL)
    {
        perror("Nie udalo sie otworzyc pliku\n");
        return 1;
    }

    fread(s, sizeof(struct superblock), 1, fp); /* wczytaj superblok */
    fseek(fp, sizeof(struct superblock), SEEK_SET);
    fread(tab, sizeof(struct inode), NR, fp); /* wczytaj tablice inode */

     for(i = 0; i < NR; ++i){ /*szukaj pierwszego bloku pliku */
        if(!tab[i].empty && tab[i].nr == 0 && tab[i].name == ename){
                count = tab[i].blocks; /* liczba blokow pliku */
                esize = tab[i].size; /*rozmiar pliku */
                k = i; /*k - index aktualnego bloku */

                printf("%d pierwszy blok\n", k);
                printf("%d blokow\n", count);
                printf("%d rozmiar\n", esize);

                for(j = 0; j < count; ++j){ /* dla kazdego bloku pliku */
                /*znajdz poczatek */
                    fseek(fp, sizeof(struct superblock) + NR * sizeof(struct inode) + k * BLOCK, SEEK_SET);

                    printf("Adres %d\n", ftell(fp));
                    printf("%d blok znaleziony\n", k);
                    fread(buffer, BLOCK, 1, fp);
                    printf("%d blok odczytany\n", k);

                    fwrite(buffer, tab[k].full, 1, efp);
                    printf("%d bajtow skopiowane\n", tab[k].full);

                    k = tab[k].next;
                    printf("%d - nastepny blok\n", k);
                }

                printf("Skopiowano\n");
                fclose(efp);
                fclose(fp);
                return 0;
            }
        }

    printf("Nie znaleziono\n");

    fclose(efp);
    fclose(fp);
    return 1;
}

int ls(FILE * fp, struct superblock * s, struct inode * tab){
    int i, j, k, count;
    fp = fopen("dysk", "rb+");
    if (fp == NULL)
    {
        perror("Nie udalo sie otworzyc dysku\n");
        return 1;
    }


    fread(s, sizeof(struct superblock), 1, fp); /* wczytaj superblok */
    fseek(fp, sizeof(struct superblock), SEEK_SET);
    fread(tab, sizeof(struct inode), NR, fp); /* wczytaj tablice inode */

    count = s->nr_of_files;

    for(i = 0; i < NR; ++i){
        if(!tab[i].empty && tab[i].nr == 0){ /*szukaj inode dla poczatku pliku */
            printf("Nazwa: %c\t", tab[i].name);
            printf("Rozmiar: %d B\n", tab[i].size);
            k = i;
            for(j = 0; j < tab[i].blocks; ++j){
                 printf("blok %d - adres %d\n", k, s->f_block + k * BLOCK);
                 k = tab[k].next;
            }
            --count;
            printf("\n");
        }

        if(count == 0)
            break;
    }

    printf("\n");
    fclose(fp);
    return 0;
}

int delete_file(FILE * fp, struct superblock * s, struct inode * tab){
    char dname;
    int i, j, k, next, count;

    fp = fopen("dysk", "rb+");
    if (fp == NULL)
    {
        perror("Nie udalo sie otworzyc dysku\n");
        return 1;
    }

    printf("Nazwa pliku do usuniecia - nazwy jednoznakowe: ");
    scanf("%s", &dname);

    fread(s, sizeof(struct superblock), 1, fp); /* wczytaj superblok */
    fseek(fp, sizeof(struct superblock), SEEK_SET);
    fread(tab, sizeof(struct inode), NR, fp); /* wczytaj tablice inode */

    printf("Jest %d plikow\n", s->nr_of_files);
    printf("%d wolnych blokow\n", s->nr_of_free);

    for(i = 0; i < NR; ++i){
        if(!tab[i].empty && tab[i].nr == 0 && tab[i].name == dname){
            count = tab[i].blocks;
            k = i; /* k - index aktualnego bloku */

            printf("%d pierwszy blok\n", k);
            printf("%d blokow\n", count);

            for(j = 0; j < count; ++j){
                next = tab[k].next;
                tab[k].empty = true;
                tab[k].name = '\0';
                tab[k].f_block = -1;
                tab[k].size = 0;
                tab[k].blocks = 0;
                tab[k].nr = -1;
                tab[k].next = -1;
                tab[k].full = 0;

                printf("Wymazano %d blok\n", k);

                k = next;

                printf("Nastepny jest %d blok\n", k);
            }

            printf("Wymazano %d blokow\n", count);
            s->nr_of_files = s->nr_of_files - 1;
            s->nr_of_free = s->nr_of_free + count;

            printf("Zostalo %d plikow\n", s->nr_of_files);
            printf("%d wolnych blokow\n", s->nr_of_free);

            fseek(fp, 0L, SEEK_SET);
            fwrite(s, sizeof(struct superblock), 1, fp);
            fwrite(tab, sizeof(struct inode), NR, fp);

            printf("Usunieto\n");
            fclose(fp);
            return 0;
        }
    }

    printf("Nie znaleziono pliku\n");
    fclose(fp);
    return 1;
}

int x_delete(){
    return remove("dysk");
}

int show_map(FILE * fp, struct superblock * s, struct inode * tab){
    int pos, i;
    char c;
    fp = fopen("dysk", "rb+");
    if (fp == NULL)
    {
        perror("Nie udalo sie otworzyc dysku\n");
        return 1;
    }

    fread(s, sizeof(struct superblock), 1, fp); /* wczytaj superblok */
    fseek(fp, sizeof(struct superblock), SEEK_SET);
    fread(tab, sizeof(struct inode), NR, fp); /* wczytaj tablice inode */

    printf("Adres\tTyp\tRozmiar\tStan\n");

    fseek(fp, 0L, SEEK_SET); /*superblok */
    pos = ftell(fp);
    printf("%d\t", pos);
    printf("S\t");
    printf("%d\t", sizeof(struct superblock));
    printf("%d, %d\n", s->nr_of_files, s->nr_of_free);

    pos = sizeof(struct superblock);
    for(i = 0; i < NR; ++i){ /*tablica inode */
        printf("%d\t", pos);
        printf("i\t");
        printf("%d\t", sizeof(struct inode));
        if(tab[i].empty)
            c = '-';
            else
            c = tab[i].name;

        printf("%c\n", c);
        pos = pos + sizeof(struct inode);
    }

    pos = sizeof(struct superblock) + NR * sizeof(struct inode); /*bloki */
    for(i = 0; i < NR; ++i){
        printf("%d\t", pos);
        printf("b\t");
        printf("%d B\t", BLOCK);
        printf("%d B\n", tab[i].full);
        pos = pos + sizeof(struct inode);
    }

    fclose(fp);
    return 0;
}

int main()
{
    FILE * f; /* deskryptor pliku */
    char c = 0; /*oznaczenie operacji */

    struct superblock super;
    struct inode tab[NR];

    /* [superblok][tablica inode [NR]][bloki danych] */

    printf("DYSK WIRTUALNY\n");
    printf("n - nowy dysk\n");
    printf("i - skopiuj plik na dysk\n");
    printf("e - skopiuj plik z dysku\n");
    printf("l - wyswietl katalog\n");
    printf("d - usun plik z dysku\n");
    printf("x - usun dysk\n");
    printf("m - wyswietl mape zajetosci\n");
    printf("q - zamknij\n");

    do{
        printf("Co chcesz zrobic? ");

        do{
        c = getchar();
        }while (c != 'n' && c != 'i' && c != 'e' && c!= 'l' && c != 'd' && c != 'x' && c != 'm' && c != 'q');

        printf("\n");

        switch (c){
        case 'n': create(f, &super); break;
        case 'i': import_copy(f, &super, tab); break;
        case 'x': x_delete(); break;
        case 'e': export_copy(f, &super, tab); break;
        case 'l': ls(f, &super, tab); break;
        case 'd': delete_file(f, &super, tab); break;
        case 'm': show_map(f, &super, tab); break;
        case 'q': return 0;
        default: printf("Niezaimplementowane\n");
        }

    }while(c != 'q');

    return 0;
}
