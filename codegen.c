/* codegen.c - machine code generation */

#include "armasm.h"

struct codegen_map_st codegen_opcode_map[] = CODEGEN_OPCODE_MAP;
struct codegen_map_st codegen_bcc_map[] = CODEGEN_BCC_MAP;

#define COND_BIT 28

void codegen_error(char *err) {
    printf("codegen_error: %s\n", err);
    exit(-1);
}

void codegen_table_init(struct codegen_table_st *ct, struct parse_node_st *tree) {
    ct->len = 0;
    ct->next = 0;
    ct->tree = tree;
	ct->label_count = 0;
	ct->public_count = 0;
}

void codegen_add_inst(struct codegen_table_st *ct, uint32_t inst) {
    ct->table[ct->len] = inst;
    ct->len += 1;
}

uint32_t codegen_lookup(char *name, struct codegen_map_st *map, int map_len) {
    int i;
    for (i = 0; i < map_len; i++) {
        if (strncmp(name, map[i].name, SCAN_TOKEN_LEN) == 0) {
            return map[i].bits;
        }
    }

    codegen_error(name);
    return (uint32_t) -1;
}

void codegen_table_add_pair(struct codegen_label_pair *lp, int index, 
                       char *label, int offset) {

    strncpy(lp[index].label, label, SCAN_TOKEN_LEN);
    lp[index].offset = offset;
}

uint32_t codegen_lookup_opcode(char *name) {
    int len = sizeof(codegen_opcode_map) / sizeof(codegen_opcode_map[0]);
    return codegen_lookup(name, codegen_opcode_map, len);
}

uint32_t codegen_lookup_bcc(char *name) {
	int len = sizeof(codegen_bcc_map) / sizeof(codegen_bcc_map[0]);
	return codegen_lookup(name, codegen_bcc_map, len);
}

int codegen_get_index(struct parse_node_st *np, char *label, int level) {
	int l1 = -1;
	int l2 = -1;
	
	if (np->type == DIR) {
		l1 = -1;
	} else if (np->type == INST) {
		if (strncmp(label, np->stmt.inst.label, SCAN_TOKEN_LEN) == 0 ) {
			l1 = level;
		} 
	} else if (np->type == SEQ) {
		l1 = codegen_get_index(np->stmt.seq.left, label, level);
		l2 = codegen_get_index(np->stmt.seq.right, label, level + 1);
		if (l1 == -1) {
			l1 = l2;
		}	
	}
	
	return l1;
}
				
void codegen_cmp_common(struct codegen_table_st *ct, uint32_t imm, uint32_t op, 
    uint32_t con, uint32_t rn, uint32_t rm) {

    const uint32_t CMP_IMM_BIT = 25;
    const uint32_t CMP_OP_BIT  = 21;
	const uint32_t CMP_CON_BIT = 20;
    const uint32_t CMP_RN_BIT  = 16;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (imm << CMP_IMM_BIT)
        | (op  << CMP_OP_BIT)
		| (con << CMP_CON_BIT)
        | (rn  << CMP_RN_BIT)
        | rm;
    codegen_add_inst(ct, inst);
}

void codegen_cmp(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_cmp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
		1, /*set condition*/
        np->stmt.inst.cmp.rn,
        np->stmt.inst.cmp.rm);
}

void codegen_cmpi(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_cmp_common(
        ct,
        1, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
		1, /*set condition*/
        np->stmt.inst.cmpi.rn,
        np->stmt.inst.cmpi.imm);
}

void codegen_dp_common(struct codegen_table_st *ct, uint32_t imm, uint32_t op, 
    uint32_t rn, uint32_t rd, uint32_t op2) {

    const uint32_t DP_IMM_BIT = 25;
    const uint32_t DP_OP_BIT  = 21;
    const uint32_t DP_RN_BIT  = 16;
    const uint32_t DP_RD_BIT  = 12;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (imm << DP_IMM_BIT)
        | (op  << DP_OP_BIT)
        | (rn  << DP_RN_BIT)
        | (rd  << DP_RD_BIT)
        | op2;
    codegen_add_inst(ct, inst);
}

void codegen_dp3(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        np->stmt.inst.dp3.rn,
        np->stmt.inst.dp3.rd,
        np->stmt.inst.dp3.rm);
}

void codegen_dp3i(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        1, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        np->stmt.inst.dp3i.rn,
        np->stmt.inst.dp3i.rd,
        np->stmt.inst.dp3i.imm);
}

void codegen_dp2(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_dp_common(
        ct,
        0, /*imm*/
        codegen_lookup_opcode(np->stmt.inst.name),
        0, /*rn*/
        np->stmt.inst.dp2.rd,
        np->stmt.inst.dp2.rm);
}

void codegen_dp2i(struct codegen_table_st *ct, struct parse_node_st *np) {
	uint32_t op = 0;
	int imm;
	
	if (np->stmt.inst.dp2i.imm < 0) {
		imm = ~(np->stmt.inst.dp2i.imm) & 0x0FF;
		op = codegen_lookup_opcode("mvn");	
	} else {
		imm = np->stmt.inst.dp2i.imm;
		op = codegen_lookup_opcode(np->stmt.inst.name);
	}

    codegen_dp_common(
        ct,
        1, /*imm*/
        op,
        0, /*rn*/
        np->stmt.inst.dp2i.rd,
        imm);
}

void codegen_lsi_common(struct codegen_table_st *ct, uint32_t imm, uint32_t rd,
    uint32_t amount, uint32_t shift, uint32_t rm) {

    const uint32_t LSI_IMM_BIT = 25;
    const uint32_t LSI_OP_BIT  = 21;
	const uint32_t LSI_RD_BIT = 12;
	const uint32_t LSI_AMOUNT_BIT = 7;
    const uint32_t LSI_SHIFT_BIT = 5;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (imm << LSI_IMM_BIT)
        | (0b1101  << LSI_OP_BIT)
		| (rd  << LSI_RD_BIT)
		| (amount << LSI_AMOUNT_BIT)
        | (shift  << LSI_SHIFT_BIT) 
        | rm;
    codegen_add_inst(ct, inst);
}

void codegen_lsi(struct codegen_table_st *ct, struct parse_node_st *np) {
	uint32_t shift = 0;
    
	if (strncmp(np->stmt.inst.name, "lsl", SCAN_TOKEN_LEN) == 0) {
        shift = 0;
    } else if (strncmp(np->stmt.inst.name, "lsr", SCAN_TOKEN_LEN) == 0) {
        shift = 1;
    } else {
		codegen_error("Invalid memory instruction.");
	}

    codegen_lsi_common(
        ct,
        0, /*imm*/
        np->stmt.inst.lsi.rd,
		np->stmt.inst.lsi.imm,
		shift,
        np->stmt.inst.lsi.rm);
}

void codegen_mul_common(struct codegen_table_st *ct, uint32_t rd, 
    uint32_t rs, uint32_t rm) {

    const uint32_t MUL_RD_BIT = 16;
    const uint32_t MUL_RS_BIT = 8;
    const uint32_t MUL_MUL_BIT = 4;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (rd  << MUL_RD_BIT)
        | (rs  << MUL_RS_BIT)
        | (0b1001  << MUL_MUL_BIT)
        | rm;
    codegen_add_inst(ct, inst);
}

void codegen_mul(struct codegen_table_st *ct, struct parse_node_st *np) {
    codegen_mul_common(
        ct,
        np->stmt.inst.mul.rd,
        np->stmt.inst.mul.rs,
        np->stmt.inst.mul.rm);
}

void codegen_mem_common(struct codegen_table_st *ct, uint32_t imm, 
    uint32_t updown, uint32_t byteword, uint32_t loadstore,
    uint32_t rn, uint32_t rd, uint32_t offset) {

    const uint32_t DP_SDT_BIT = 26;
    const uint32_t DP_IMM_BIT = 25;
    const uint32_t DP_PREPOST_BIT = 24;
    const uint32_t DP_UPDOWN_BIT = 23;
    const uint32_t DP_BYTEWORD_BIT = 22;
    const uint32_t DP_LOADSTORE_BIT = 20;    
    const uint32_t DP_RN_BIT  = 16;
    const uint32_t DP_RD_BIT  = 12;
    uint32_t inst = 0;

    inst = (COND_AL << COND_BIT)
        | (0b01 << DP_SDT_BIT)
        | (imm << DP_IMM_BIT)
        | (0b1 << DP_PREPOST_BIT)
        | (updown << DP_UPDOWN_BIT)
        | (byteword << DP_BYTEWORD_BIT)
        | (loadstore << DP_LOADSTORE_BIT)
        | (rn  << DP_RN_BIT)
        | (rd  << DP_RD_BIT)
        | offset;
    codegen_add_inst(ct, inst);
}

void codegen_mem3(struct codegen_table_st *ct, struct parse_node_st *np) {
    uint32_t loadstore = 0;
 	uint32_t byteword = 0;

    if (strncmp(np->stmt.inst.name, "str", SCAN_TOKEN_LEN) == 0) {
        loadstore = 0;
		byteword = 0;
    } else if (strncmp(np->stmt.inst.name, "ldr", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
		byteword = 0;
    } else if (strncmp(np->stmt.inst.name, "strb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 0;
		byteword = 1;
    } else if (strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
		byteword = 1;
    } else {
        codegen_error("Invalid memory instruction.");
    }
	
	codegen_mem_common(
        ct,
        1, /* imm */
        1, /* updown */
        byteword, /* byteword */
        loadstore, /* loadstore */
        np->stmt.inst.mem3.rn,
        np->stmt.inst.mem3.rd,
        np->stmt.inst.mem3.rm);
}

void codegen_memi(struct codegen_table_st *ct, struct parse_node_st *np) {
    uint32_t loadstore = 0;
	uint32_t byteword = 0;

    if (strncmp(np->stmt.inst.name, "str", SCAN_TOKEN_LEN) == 0) {
        loadstore = 0;
		byteword = 0;
    } else if (strncmp(np->stmt.inst.name, "ldr", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
		byteword = 0;
    } else if (strncmp(np->stmt.inst.name, "strb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 0;
		byteword = 1;
    } else if (strncmp(np->stmt.inst.name, "ldrb", SCAN_TOKEN_LEN) == 0) {
        loadstore = 1;
		byteword = 1;
    } else {
        codegen_error("Invalid memory instruction.");
    }
	
    codegen_mem_common(
        ct,
        0, /* imm*/
        1, /* updown */
        byteword, /* byteword */
        loadstore, /* loadstore */
        np->stmt.inst.memi.rn,
        np->stmt.inst.memi.rd,
        np->stmt.inst.memi.imm);
}


void codegen_bx(struct codegen_table_st *ct, struct parse_node_st *np) {
    const uint32_t BX_CODE_BIT = 4;
    const uint32_t bx_code = 0b000100101111111111110001;

    uint32_t inst = (COND_AL << COND_BIT)
        | (bx_code << BX_CODE_BIT)
        | np->stmt.inst.bx.rn;
    codegen_add_inst(ct, inst);
}

void codegen_b_common(struct codegen_table_st *ct, uint32_t cond, uint32_t lbit, 
    uint32_t offset) {

    const uint32_t B_B_BIT  = 25;
    const uint32_t B_LINK_BIT  = 24;
    uint32_t inst = 0;

    inst = (cond << COND_BIT)
        | (0b101 << B_B_BIT)
        | (lbit  << B_LINK_BIT)
        | offset;
    codegen_add_inst(ct, inst);
}

void codegen_b(struct codegen_table_st *ct, struct parse_node_st *np) {
	uint32_t lbit = 0;
	int index = ct->len;
	int target_index = codegen_get_index(ct->tree, np->stmt.inst.b.label, 0);
	uint32_t offset = target_index - (index + 2) - ct->public_count;

	offset = offset & 0x00FFFFFF;	
	
	if (strncmp(np->stmt.inst.name, "bl", SCAN_TOKEN_LEN) == 0) {
        lbit = 1;
    } else {
		lbit = 0;
	}

	codegen_b_common(
		ct,
		codegen_lookup_bcc(np->stmt.inst.name),
		lbit,
		offset);
}

void codegen_inst(struct codegen_table_st *ct, struct parse_node_st *np) {
	
	if (strlen(np->stmt.inst.label) > 0) {
		int offset = ct->len * 4;
		codegen_table_add_pair(ct->labels, ct->label_count, np->stmt.inst.label, offset);
		ct->label_count += 1;
	} 
    switch (np->stmt.inst.type) {
		case CMPI : codegen_cmpi(ct, np); break;
		case CMP  : codegen_cmp(ct, np); break;
        case DP3  : codegen_dp3(ct, np); break;
		case DP3I : codegen_dp3i(ct, np); break;
		case DP2  : codegen_dp2(ct, np); break;
		case DP2I : codegen_dp2i(ct, np); break;
		case LSI  : codegen_lsi(ct, np); break;
		case MUL  : codegen_mul(ct, np); break;
        case MEM3 : codegen_mem3(ct, np); break;
        case MEMI : codegen_memi(ct, np); break;
		case B    : codegen_b(ct, np); break;
        case BX   : codegen_bx(ct, np); break;
        default   : codegen_error("unknown stmt.inst.type");
    }
}

void codegen_stmt(struct codegen_table_st *ct, struct parse_node_st *np) {
	
	if (np->type == DIR) {
		codegen_table_add_pair(ct->publics, ct->public_count, np->stmt.dir.label, 0x0);
		ct->public_count += 1;
	} else if (np->type == INST) {
        codegen_inst(ct, np);
    } else if (np->type == SEQ) {
        codegen_stmt(ct, np->stmt.seq.left);
        codegen_stmt(ct, np->stmt.seq.right);
    }
}

void codegen_print_hex(struct codegen_table_st *ct) {
    int i;

    printf("v2.0 raw\n");
    for (i = 0; i < ct->len; i++) {
        printf("%08X\n", ct->table[i]);
    }
}

void codegen_write(struct codegen_table_st *ct, char *path) {
    int i;
    FILE *obj = fopen(path, "w");

    fprintf(obj, "v2.0 raw\n");
    for (i = 0; i < ct->len; i++) {
        fprintf(obj, "%08X\n", ct->table[i]);
    }
    fclose(obj);
}

void codegen_hex_write(struct codegen_table_st *ct, struct parse_node_st *np, char *path) {
    codegen_write(ct, path);
}
