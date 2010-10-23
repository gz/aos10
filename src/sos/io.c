/**
 * Walks through the file table and finds the first free entry in file table.
 *
 * @return This function returns the index of the first free entry found or -1
 * if the file table is currently full.
 */
static int find_free_file_slot() {
	for(int i=0; i< MAX_FILES; i++) {
		if(file_table[i] == NULL)
			return i;
	}

	return -1; // file table is full
}


/**
 *
 * @param tid
 * @param msg_p
 */
void open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p) {

}

/**
 *
 * @param tid
 * @param msg_p
 */
void read_file(L4_ThreadId_t tid, L4_Msg_t* msg_p) {

}

/**
 *
 * @param tid
 * @param msg_p
 */
void write_file(L4_ThreadId_t tid, L4_Msg_t* msg_p) {

}
