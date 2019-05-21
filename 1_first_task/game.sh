#!/bin/bash

#global variable pipe = FIFO


#COLORS:
#0 = black, 1 = red, 2 = green, 3 = brown, 4 = blue
#5 = purple, 6 = cyan, 7 = bright grey, 8 = grey
#9 = bright red, 10 = bright green, 11 = bright yellow, 12 = bright blue
#13 = bright purple, 14 = very bright blue, 15 = white


MAP='         '
POINTER_X=0
POINTER_Y=0
PLAYER_CHAR='x'
get_opponent_char='o'
PLAYERS_COUNT=2
#BORDER='░'
STANDOUT_UNDERLINE_MODE=`tput sgr 1 1`
BANNER_COLOR=`tput setaf 5`
BORDER_COLOR=`tput setaf 1`
PLAYER_COLOR=`tput setaf 2`
OPPONENT_COLOR=`tput setaf 1`
POINTER_COLOR=`tput setaf 2`
GREEN_COLOR=`tput setaf 10`
RED_COLOR=`tput setaf 9`
NORMAL_MODE=`tput sgr 0 0`

function print_banner() {
    echo ${BANNER_COLOR}
    echo '------------------------'
    echo '|▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓|'
    echo '|▓                    ▓|'
    echo '|▓krestiki-noliki 2019▓|'
    echo '|▓                    ▓|'
    echo '|▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓|'
    echo '------------------------'
    if [[ $PLAYER_CHAR = ${get_opponent_char} ]];
        then echo ${STANDOUT_UNDERLINE_MODE}${GREEN_COLOR}
        else echo ${RED_COLOR}
    fi
    echo 'Вы играете за : '${PLAYER_CHAR}${NORMAL_MODE}$'\n$\n'
}

function refresh_map() {
    #в бесконечном цикле отрисовываем карту (цикл в конце скрипта)
    tput reset
    print_banner
    echo $BORDER_COLOR'░░░░░░'
    for x in 0 1 2; do
        for y in 0 1 2; do
            m=${MAP:3 * x + y:1}
            if [[ $x = $POINTER_Y ]] && [[ $y = $POINTER_X ]];
                then echo -n $POINTER_COLOR$PLAYER_CHAR$NORMAL_MODE$POINTER_COLOR'<'$BORDER_COLOR
                else echo -n $NORMAL_MODE${m}$BORDER_COLOR'░'$BORDER_COLOR
            fi
        done
        echo ' '
        echo '░░░░░░'
    done
    echo $n
}

function draw_character() {
    #x_coord in $2, y_coord in $3
    cursor=$((3 * $3 + $2))

    if [[ ${MAP:cursor:1} = ' ' ]]; then
        MAP=${MAP:0:cursor}${1}${MAP:cursor + 1}
    fi
}

function make_turn() {
    draw_character $PLAYER_CHAR $POINTER_X $POINTER_Y
    echo $POINTER_X $POINTER_Y > pipe
    get_opponent_char=`get_opponent_char`
}

function key_handler() {
    #key in $1
    case $1 in
        A) POINTER_Y=$(((POINTER_Y + 2) % 3));;
        B) POINTER_Y=$(((POINTER_Y + 1) % 3));;
        C) POINTER_X=$(((POINTER_X + 1) % 3));;
        D) POINTER_X=$(((POINTER_X + 2) % 3));;
        '') make_turn;;
    esac
}

function make_player_move() {
    read -r -sn1 key
    key_handler $key
}

function make_opponent_move() {
    opponent_move=`cat pipe`
    draw_character `get_opponent_char` $opponent_move
    get_opponent_char=$PLAYER_CHAR
}

function game_handler() {
    #$1 == turn has been made by player
    if [[ $1 = $PLAYER_CHAR ]]; 
        then make_player_move
        else make_opponent_move
    fi
}

function game_has_finished() {
    # возможные комбинации для победы
    win_indexes=(0 1 2 3 4 5 6 7 8 0 3 6 1 4 7 2 5 8 0 4 8 2 4 6)
    # >opennet: ${#array[*]} и ${#array[@]} возвращает количество элементов в массиве.
    for i in "${#win_indexes[@]} - 1"
    do
        first=${MAP:win_indexes[i]:1}
        second=${MAP:win_indexes[i + 1]:1}
        third=${MAP:win_indexes[i + 2]:1}
        if [[ $first = $second ]] && [[ $first != '-' ]] && [[ $second = $third ]]; then
            echo $first
            break
        fi
        i = $i+3
    done
}

function get_random_player() {
    # 2 игрока - x, o
    player_index=$((RANDOM % $PLAYERS_COUNT))
    if [[ $player_index = 0 ]];
        then echo 'x';
        else echo 'o';
    fi
}

function get_opponent_char() {
    if [[ $PLAYER_CHAR = 'x' ]];
        then echo 'o'
        else echo 'x'
    fi
}

function first_connection() {
    #trap 'reset; echo До свидания!; sleep 2; rm pipe; reset' SIGINT SIGTERM SIGHUP SIGQUIT
    trap 'rm pipe; reset' EXIT
    echo ${BANNER_COLOR}'Ожидание игроков...'
    PLAYER_CHAR=`get_random_player`
    echo `get_opponent_char` > pipe
    opponent_pid=`cat pipe`
    trap 'kill -INT -'$opponent_pid' &>/dev/null; reset; exit' INT
    echo $$ > pipe
}

function second_connection() {
    PLAYER_CHAR=`cat pipe`
    echo $$ > pipe
    opponent_pid=`cat pipe`
    trap 'kill -INT -'$opponent_pid' &>/dev/null; reset; exit' INT
    trap 'reset' EXIT
}

function check_winner() {
    winner=`game_has_finished`

    if [[ ! $MAP =~ " " ]] || [[ $winner != '' ]]; then
        if [[ $winner = $PLAYER_CHAR ]];
            then echo ${GREEN_COLOR}'Победитель!'
        elif [[ $winner = `get_opponent_char` ]];
            then echo ${RED_COLOR}'Проигравший!'
        else
            echo 'Ничья!'
        fi
        sleep 2
        exit
    fi
}

function make_connection() {
    #$1 = return
    if [[ $1 = 0 ]];
        #если никто не подключился
        then first_connection
        #иначе ждем второго
        else second_connection
    fi
}

#trap '' SIGWINCH
mkfifo pipe &>/dev/null
make_connection $?

while true; do
    refresh_map
    game_handler $get_opponent_char
    check_winner
done

