## Usage

`pf [-u|-c|-a] [-n n] [-s] commande [argument...]`

L’utilitaire exécute une commande, éventuellement plusieurs fois en parallèle et/ou en série, et affiche le temps réel et/ou utilisateur consommé par la commande et/ou le nombre d'instructions machines utilisés par la commande.

C'est une version simplifiée des utilitaires `time` et `perf`.

###

Par défaut, `pf` présente le nombre de secondes écoulées en temps réel par la commande (deux chiffre de précision après le point).
L'affichage se fait sur la sortie standard d'erreur.

```
$ ./pf ./work
1.00
```

Ceci est analogue à ce que retourne la commande `time` avec `%e`.

```
$ command time -f "%e" ./work
1.00
```

Bien sur, la commande peut être n'importe quel programme valide et ses arguments.

```
$ ./pf sleep 1.5
1.50
```

### -u

Avec l'option `-u`, `pf` présente le nombre de secondes écoulées en temps utilisateur par la commande (deux chiffre de précision après le point)

```
$ ./pf -u ./work
0.66
$ ./pf -u sleep 1
0.00
```

Ceci est analogue à ce que retourne la commande `time` avec `%u`.

```
$ command time -f "%U" ./work
0.66
$ command time -f "%U" sleep 1
0.00
```

### -c

Avec l'option `-c`, `pf` présenter le nombre de cycles processeur consommés par la commande en mode utilisateur.

```
$ ./pf -c ./work
140938200
```

Ceci est analogue au compteur `cycles` de `perf stat`. C'est le premier nombre affiché par la commande de l'exemple suivant.

```
$ perf stat -e cycles -x, ./work
142304586,,cycles:u,1000201099,100,00,,
```

### -a

Avec l'option `-a`, l'utilitaire `pf` affiche les trois valeurs : le temps réel, le temps utilisateur et le nombre de cycles processeurs.

```
$ ./pf -a ./work
1.00 0.61 143603651
$ ./pf -a sleep 1.1
1.10 0.00 1211159
```


### -n

L'option `-n` indique un nombre de répétition.
La commande est ainsi exécutée autant de fois qu'indiqué avec un affichage des valeurs de chaque exécution.

Si n est supérieur à 1, une ligne supplémentaire est affichée en dernier qui contient les valeurs moyennes de l'ensemble des exécutions.

```
./pf -n 5 ./work
1.00
1.00
0.99
1.00
1.00
1.00
```

Les commandes sont exécutées l'une à la suite de l'autre.
Ainsi l'exemple précédent a pris 5 secondes de temps réel pour s'exécuter.

On pourrait utiliser `time` pour mesurer le temps réel disponible mais `./pf` fait aussi l'affaire.
Par exemple, un premier `pf` exécute et mesure un second `pf` qui exécute deux `sleep 1` :

```
./pf ./pf -n 2 sleep 1
1.00
1.00
1.00
2.00
```

`-n` est compatible avec les autres options de l'outil.

```
./pf -a -n 3 sleep 1.1
1.10 0.00 454595
1.10 0.00 441480
1.10 0.00 430592
1.10 0.00 442222
```

### -s

L'option `-s` exécute la commande dans un sous shell `/bin/sh` via l'option `-c`.
Cela permet de mesurer des commandes complexes.
Bien sur, le cout du shell est mesuré aussi.

```
./pf -s './work; sleep .5'
1.50
```


`-s` est compatible avec les autres options de l'outil.

```
./pf -a -n 3 -s './work& ./work; wait'
1.00 1.32 278797959
1.00 1.34 278431654
1.00 1.33 275326470
1.00 1.33 277518694
```
