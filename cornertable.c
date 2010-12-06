/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2009 Andrew Brown <brownan@gmail.com>
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cornertable.h"
#include "common.h"
#include "stack.h"
#include "cube.h"

/*
 * Maps corner locations to indexes 0-7.
 */
int corner_index[] = {
    [0]  = 0,
    [2]  = 1,
    [5]  = 2,
    [7]  = 3,
    [12] = 4,
    [14] = 5,
    [17] = 6,
    [19] = 7
};

/*
 * Used to calculate the corner cubie's value in the variable radix number
 * system.  The first call to this function will yield a number from 0-7
 * depending on the cubie chosen.  The next call will yield 0-6, then 0-5, etc.
 * This is accomplished by decrementing the value of every slot after the
 * position given.
 */
int corner_value(int position, int corner_slot_value[])
{
    // Convert position to a number 0-7
    position = corner_index[position];

    // Decrement the value of all positions after this one
    int i;
    for (i=position+1; i<=7; i++) {
        corner_slot_value[i]--;
    }

    return corner_slot_value[position];
}

/*
 * cornertable.c
 * this has functions to generate the 88179840 element corner heuristics table,
 * as well as methods to use it
 */

/*
 * corner_map
 * Input: a cube_type string
 * Output: An integer in the range 0 to 88179840-1
 *
 * TODO
 */
int corner_map(const char *cubestr)
{
    int index;

    // Positions
    // Determine a value 
    int corner_slot_value[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int position_value[7];
    position_value[0] = corner_value(CUBIE(cubestr, 0)[0], corner_slot_value);
    position_value[1] = corner_value(CUBIE(cubestr, 2)[0], corner_slot_value);
    position_value[2] = corner_value(CUBIE(cubestr, 5)[0], corner_slot_value);
    position_value[3] = corner_value(CUBIE(cubestr, 7)[0], corner_slot_value);
    position_value[4] = corner_value(CUBIE(cubestr, 12)[0], corner_slot_value);
    position_value[5] = corner_value(CUBIE(cubestr, 14)[0], corner_slot_value);
    position_value[6] = corner_value(CUBIE(cubestr, 17)[0], corner_slot_value);

    // Positions
    index =  position_value[0] *3*3*3*3*3*3*3 * 2*3*4*5*6*7; // (0:7) * base
    index += position_value[1] *3*3*3*3*3*3*3 * 2*3*4*5*6;   // (0:6) * base
    index += position_value[2] *3*3*3*3*3*3*3 * 2*3*4*5;     // (0:5) * base
    index += position_value[3] *3*3*3*3*3*3*3 * 2*3*4;       // (0:4) * base
    index += position_value[4] *3*3*3*3*3*3*3 * 2*3;         // (0:3) * base
    index += position_value[5] *3*3*3*3*3*3*3 * 2;           // (0:2) * base
    index += position_value[6] *3*3*3*3*3*3*3;               // (0:1) * base

    // Orientations
    index += CUBIE(cubestr,0)[1]  *3*3*3*3*3*3; // (0:3) * base
    index += CUBIE(cubestr,2)[1]  *3*3*3*3*3;   // (0:3) * base
    index += CUBIE(cubestr,5)[1]  *3*3*3*3;     // (0:3) * base
    index += CUBIE(cubestr,7)[1]  *3*3*3;       // (0:3) * base
    index += CUBIE(cubestr,12)[1] *3*3;         // (0:3) * base
    index += CUBIE(cubestr,14)[1] *3;           // (0:3) * base
    index += CUBIE(cubestr,17)[1];              // (0:3) * base

#ifdef DEBUG_ASSERTS
    if (index >= 88179840 || index < 0) {
        fprintf(stderr, "\nWARNING: HASH RETURNED %d\n", index);
        index = *((int *)0x0); /* sigsev */
    }
#endif
    return index;

}

/*
 * Generates the corner heuristics table about the given solution, which could
 * be any valid cube.  The table is stored in the given character pointer,
 * which should be allocated with the CORNER_TABLE_NEW macro
 * return 1 on success
 */
int corner_generate(unsigned char *cornertable, const char *solution)
{
    stacktype *stack;
    /*
     * qdata is a struct, holding cube string 'cube_data',
     * an int representing the turn that was made, 'turn',
     * and an int representing the distance, 'distance'
     */
    qdata current;
    int count = 0; /* total hashed */
    int popcount = 0; /* total traversed */
    int hash;
    int i;
    unsigned char *instack;
    int depth;
    cube_type turned;

    /* Create a stack */
    stack = STACK_NEW;

    /*
     * create a temporary table to keep track of the stack.  This holds the
     * value of each item that has been added to the stack, and its distance.
     * This way, we know if we encounter the same node but at a further
     * distance we can throw it away.  This heuristic cuts down on processing
     * time by quite a bit, and is only as time intensive as the hashing
     * algorithm, at the expense of using more memory.
     */
    instack = CORNER_TABLE_NEW;

    depth = -1;
    while (count < 88179840)
    {
        /* if stack is empty, go up a level */
        if (stack->length == 0)
        {
            stack_push(stack, solution, -1, 0);
            depth++;
            /* clear out instack table */
            CORNER_TABLE_CLEAR(instack);
        }

        /* Pop the first item off, put it in current */
        stack_peek_cube(stack, &(current.cube_data));
        current.turn = stack_peek_turn(stack);
        current.distance = stack_peek_distance(stack);
        stack_pop(stack);
        popcount++;

        /* Print out status every 2^18 pops  (approx every 200k)*/
        if ((popcount & 0777777) == 0777777) {
            fprintf(stderr, "\r%d/88179840 hashed, on level:%d/11, total traversed:%d %d%% ", count, depth, popcount, popcount/5788130);
        }
#ifdef PROFILE_MODE
        /* For profiling, so I don't have to wait an hour to gather data */
        if (count == 10000000) {
            return 0;
        }
#endif

        /*
         * if item is at our current target depth, add it to hash table
         */
        if (current.distance == depth) {
            hash = corner_map(current.cube_data);
            if (hash & 1) {
                if (!(cornertable[(hash-1)/2] >> 4)) {
                    cornertable[(hash-1)/2] |= current.distance << 4;
                    count++;
                } else {
                    /* A duplicate, skip */
                }
            } else {
                if (!(cornertable[hash/2]&15)) {
                    cornertable[hash/2] |= current.distance;
                    count++;
                } else {
                    /* a duplicate */
                }
            }
        } else {
            /* Not at the current depth, put all turns onto the stack */
            for (i=0; i<18; i++) {
                /* Determine if we should skip this turn */
                if (current.turn != -1 && SHOULDIAVOID(i, current.turn)) {
                    continue;
                }

                memcpy(turned, current.cube_data, CUBELEN);
                cube_turn(turned, i);

                /*
                 * Check if turned is in instack and is greater than
                 * or equal to the depth.  If so, skip
                 */
                hash = corner_map(turned);
                if (hash&1 ? \
                        ((instack[(hash-1)/2] >> 4) <= (current.distance+1)) : \
                        ((instack[hash/2] & 15) <= (current.distance+1))) {
                    continue;
                }
                /* add to instack */
                if (hash&1) {
                    instack[(hash-1)/2] &= 15;
                    instack[(hash-1)/2] |= (current.distance+1) << 4;
                } else {
                    instack[hash/2] &= 15<<4;
                    instack[hash/2] |= (current.distance+1);
                }

                /* Add to real stack */
                stack_push(stack, turned, i, current.distance+1);
            }
        }

    }
    free(instack);
    while(stack->length) {
        stack_pop(stack);
    }
    free(stack);
    fprintf(stderr, "\n");
    return 1;
}

/*
 * These methods read and write the given table to/from the given
 * FILE handle
 * Return 1 on success 0 on failure
 */
int corner_write(unsigned char *cornertable, FILE *output)
{
    int written;
    written = fwrite(cornertable, 1, 44089920, output);
    if (written < 44089920)
        return 0;
    return 1;
}
int corner_read(unsigned char *cornertable, FILE *input)
{
    int read;
    read = fread(cornertable, 1, 44089920, input);
    if (read < 44089920) {
        return 0;
    }
    return 1;
}
