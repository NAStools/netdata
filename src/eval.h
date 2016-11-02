#ifndef NETDATA_EVAL_H
#define NETDATA_EVAL_H

#define EVAL_MAX_VARIABLE_NAME_LENGTH 300

typedef struct eval_variable {
    char *name;
    uint32_t hash;
    struct rrdvar *rrdvar;
    struct eval_variable *next;
} EVAL_VARIABLE;

typedef struct eval_expression {
    const char *source;
    const char *parsed_as;

    int *status;
    calculated_number *this;
    time_t *after;
    time_t *before;

    calculated_number result;

    int error;
    BUFFER *error_msg;

    // hidden EVAL_NODE *
    void *nodes;

    // custom data to be used for looking up variables
    struct rrdcalc *rrdcalc;
} EVAL_EXPRESSION;

#define EVAL_VALUE_INVALID    0
#define EVAL_VALUE_NUMBER     1
#define EVAL_VALUE_VARIABLE   2
#define EVAL_VALUE_EXPRESSION 3

// parsing and evaluation
#define EVAL_ERROR_OK                             0

// parsing errors
#define EVAL_ERROR_MISSING_CLOSE_SUBEXPRESSION    1
#define EVAL_ERROR_UNKNOWN_OPERAND                2
#define EVAL_ERROR_MISSING_OPERAND                3
#define EVAL_ERROR_MISSING_OPERATOR               4
#define EVAL_ERROR_REMAINING_GARBAGE              5
#define EVAL_ERROR_IF_THEN_ELSE_MISSING_ELSE      6

// evaluation errors
#define EVAL_ERROR_INVALID_VALUE                101
#define EVAL_ERROR_INVALID_NUMBER_OF_OPERANDS   102
#define EVAL_ERROR_VALUE_IS_NAN                 103
#define EVAL_ERROR_VALUE_IS_INFINITE            104
#define EVAL_ERROR_UNKNOWN_VARIABLE             105

// parse the given string as an expression and return:
//   a pointer to an expression if it parsed OK
//   NULL in which case the pointer to error has the error code
extern EVAL_EXPRESSION *expression_parse(const char *string, const char **failed_at, int *error);

// free all resources allocated for an expression
extern void expression_free(EVAL_EXPRESSION *op);

// convert an error code to a message
extern const char *expression_strerror(int error);

// evaluate an expression and return
// 1 = OK, the result is in: expression->result
// 2 = FAILED, the error message is in: buffer_tostring(expression->error_msg)
extern int expression_evaluate(EVAL_EXPRESSION *expression);

#endif //NETDATA_EVAL_H
