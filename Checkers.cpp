#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <algorithm>
#include <random>

#define NIY assert(0 && "Not implemented yet!");

using namespace std;

class Board{
public:
	enum tile : uint8_t{ EMPTY = 0, BLACK, WHITE, QBLACK, QWHITE, COUNT };
	class State{
	public:
		tile* t = NULL;
		int size;
		struct _State{
			State* s;
			int j;
		public:
			_State(State* s, int j) :s(s), j(j){}
			inline const char operator()(){ assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT); return " xoXO+"[s->t[j]]; }
			inline _State operator= (tile v){
				assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT);
				s->t[j] = v;
				return *this;
			}
			inline operator tile(){
				assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT);
				return s->t[j];
			}
			inline bool operator== (tile v){ if(j >= 0 && j < s->size)return s->t[j] == v; else return v == COUNT; }
			inline bool operator!= (tile v){ if(j >= 0 && j < s->size)return s->t[j] != v; else return v != COUNT; }
		};
		State():size(0){}
		State(int size):size(size){ t = new tile[size];
			for(int i = 0; i < size; i++){
				t[i] = EMPTY;
			}
		}
		inline _State operator[] (int i){
			return _State(this, i);
		}
	};
	class Move{
	public:
		bool jump;
		uint8_t pos;
		uint8_t over;
		uint8_t dst;
		Move* seq;
		Move() :jump(true), pos(0), over(0), dst(0), seq(NULL){}
		Move(uint8_t pos, uint8_t over, uint8_t dst, bool jump, Move* seq):jump(jump), dst(dst), over(over), pos(pos), seq(seq){}
	};
	State t;
	int size;
	Board(int size):size(size){
		t = State(size * size);
		for(int i = 0; i < size * 2; i++){
			if((i+i/8) % 2 == 0){
				t[i] = WHITE;
				t[size * size - i - 1] = BLACK;
			}
		}
	}
	~Board(){ delete[] t.t; }
	void print(){
		for(int i = 0; i < size; i++){
			cout << "+-+-+-+-+-+-+-+-+\n";
			for(int j = 0; j < size; j++){
				cout << "|" << t[j + i * size]();
			}
			cout << "|\n";
		}
		cout << "+-+-+-+-+-+-+-+-+\n";
	}
	vector<Move>* generate_moves(bool side){//side black=0, white=1
		int p = possible_jumps(side);
		if(p!=0){
			cout << "Must jump" << endl;
			return generate_jumps(side,p);
		}
		return generate_slides(side);
	}
	inline string pretty_pos(int pos){
		return "["+to_string(pos%size)+","+to_string(pos/size)+"]";
	}
	constexpr inline int delta_dir(const int dir){
		//0=^\ 1=/^ 2=\v 3=v/ 
		assert(dir >= 0 && dir < 4);
		return (dir % 3 == 0 ? -1 : 1) + (size * (dir < 2 ? -1 : 1));
	}
	inline bool bounds(int pos, int diff, int dist){
		//if(dist == 7) printf("bounds():qjump");
		return (abs(pos % size - (pos + diff*dist) % size) == dist && abs(pos / size - (pos + diff*dist) / size) == dist);
	}

	inline bool can_move(int pos, int dir, int max){
		//0=-1,-1 1=+1,-1 2=+1,+1 3=-1,+1
		int diff = delta_dir(dir);
		if(max == 0)return t[pos + diff] == EMPTY && bounds(pos, diff, 1);
		if(max == 1)return (t[pos + diff] != EMPTY && t[pos + diff] != COUNT && t[pos + diff * 2] == EMPTY) && bounds(pos,diff,2);
		if(max > 1){
			for(int i = 1; i < max; i++){
				if(t[pos + diff * i] != EMPTY)return false;//TODO:bounds doesnt work properly
			}
			return bounds(pos,diff,max);
		}
		assert(0 && "Unreachable");
		return false;
	}

	inline bool can_jump_s(bool side, int pos){
		if(!side){//black
			return can_move(pos, 0, 1) && (t[pos + delta_dir(0)] == WHITE || t[pos + delta_dir(0)] == QWHITE)
				|| can_move(pos, 1, 1) && (t[pos + delta_dir(1)] == WHITE || t[pos + delta_dir(1)] == QWHITE);
		} else{//white
			return can_move(pos, 2, 1) && (t[pos + delta_dir(2)] == BLACK || t[pos + delta_dir(2)] == QBLACK)
				|| can_move(pos, 3, 1) && (t[pos + delta_dir(3)] == BLACK || t[pos + delta_dir(3)] == QBLACK);
		}
	}
	inline bool can_jump_q(bool side, int pos){
		for(int dir = 0; dir < 4; dir++){
			if(can_jump_q(side, pos, dir)) return true;
		}
		return false;
	}
	inline bool can_jump_q(bool side, int pos, int dir){
		int diff = delta_dir(dir);
		for(int i = 1; i < size; i++){
			if(t[pos + diff * i] == (tile) (BLACK + side) || t[pos + diff * i] == (tile) (QBLACK + side) || t[pos + diff * i] == COUNT || !bounds(pos,diff,i))break;
			if(t[pos + diff * i] == EMPTY)continue;
			if((t[pos + diff * i] == (tile) (BLACK + !side) || t[pos + diff * i] == (tile) (QBLACK + !side))
				&& t[pos + diff * (i + 1)] == EMPTY)return true;
		}//TODO:check
		return false;
	}
	inline int possible_jumps(bool side){
		for(int i = 0; i < size * size; i++){
			if(t[i] == (tile) (QBLACK + side)){
				if(can_jump_q(side,i))return 2;
			}else if(t[i] == (tile) (BLACK + side)){
				if(can_jump_s(side,i))return 1;
			}
		}
		return 0;
	}

	Move* gen_jump_s(int pos, int dir){
		if(can_move(pos, dir, 1)){
			Move* x = new Move(pos,pos+delta_dir(dir),pos+delta_dir(dir)*2,true,NULL);
			Move* seq0 = gen_jump_s(pos + delta_dir(2) * 2, dir<=1?0:2);
			Move* seq1 = gen_jump_s(pos + delta_dir(2) * 2, dir<=1?1:3);
			if(seq0 != NULL && seq1 != NULL){
				x->seq = new Move[3]();
				x->seq[0] = *seq0;
				delete seq0;
				x->seq[1] = *seq1;
				delete seq1;
			}else if(seq0 != NULL){
				x->seq = new Move[2]();
				x->seq[0] = *seq0;
				delete seq0;
			}else if(seq1 != NULL){
				x->seq = new Move[2]();
				x->seq[0] = *seq1;
				delete seq1;
			}
			return x;
		}else{
			return NULL;
		}
	}

	Move* gen_jump_q(int pos, int dir){
		int diff = delta_dir(dir);
		Move* m = new Move[size-2]();
		int k = 0;
		for(int i = 1; i < size; i++){
			if(t[pos + diff * i] != EMPTY && t[pos + diff * i] != COUNT){
				for(int j = 0; j < size - 2; j++){
					m[j] = Move(pos, pos + diff * i, 0, false, NULL);
				}
				for(++i; i < size; i++){
					if(t[pos + diff * i] == EMPTY && bounds(pos, diff, i)){
						m[k].dst = pos + diff * i;
						m[k].jump = true;
						k++;
					} else{
						return m;
					}
				}
				return m;
			}
		}
		assert(0 && "Unreachable");
		return NULL;
	}

	vector<Move>* generate_jumps(bool side, int type){
		//assume jump is possible
		//queen jumps first, then stones, length ignored but the whole sequence
		vector<Move>* moves = new vector<Move>();
		for(int i = 0; i < size * size; i++){
			Move* x;
			switch (t[i]){
			case QBLACK:
				if(!side){
					for(int dir = 0; dir < 4; dir++){
						if(can_jump_q(side, i, dir)){
							x = gen_jump_q(i, dir);
							if(x != NULL){
								for(int j = 0; j < size - 2; j++){
									if(x[j].jump) moves->push_back(x[j]);
								}
								delete[] x;
							}
						}
					}
				}
				break;
			case QWHITE:
				if(side){
					for(int dir = 0; dir < 4; dir++){
						if(can_jump_q(side, i, dir)){
							x = gen_jump_q(i, dir);
							if(x != NULL){
								for(int j = 0; j < size - 2; j++){
									if(x[j].jump) moves->push_back(x[j]);
								}
								delete[] x;
							}
						}
					}
				}
				break;
			case BLACK:
				if(!side && type != 2){
					if(t[i + delta_dir(0)] == WHITE || t[i + delta_dir(0)] == QWHITE){
						x = gen_jump_s(i, 0);
						if(x != NULL){
							moves->push_back(*x);
							//if(x->seq!=NULL) delete[] x->seq;
							delete x;
						}
					}
					if(t[i + delta_dir(1)] == WHITE || t[i + delta_dir(1)] == QWHITE){
						x = gen_jump_s(i, 1);
						if(x != NULL){
							moves->push_back(*x);
							//if(x->seq!=NULL) delete[] x->seq;
							delete x;
						}
					}
				}
				break;
			case WHITE:
				if(side && type != 2){
					if(t[i + delta_dir(2)] == BLACK || t[i + delta_dir(2)] == QBLACK){
						x = gen_jump_s(i, 2);
						if(x != NULL){
							moves->push_back(*x);
							//if(x->seq!=NULL) delete[] x->seq;
							delete x;
						}
					}
					if(t[i + delta_dir(3)] == BLACK || t[i + delta_dir(3)] == QBLACK){
						x = gen_jump_s(i, 3);
						if(x != NULL){
							moves->push_back(*x);
							//if(x->seq!=NULL) delete[] x->seq;
							delete x;
						}
					}
				}
				break;
			case EMPTY:
				break;
			default:
				assert(0 && "Unreachable");
			}
		}
		return moves;
	}

	vector<Move>* generate_slides(bool side){
		vector<Move>* moves = new vector<Move>();
		for(int i = 0; i < size * size; i++){
			if(t[i] == (tile) (QBLACK + side)){
				for(int dir = 0; dir < 4; dir++){
					for(int j = 1; j < size; j++){
						int diff = delta_dir(dir);
						if(can_move(i, dir, max(2, j))){
							moves->push_back(Move(i, 0, i + diff * j, false, NULL));
						} else{
							break;
						}
					}
				}
			} else if(t[i] == (tile) (BLACK + side)){
				if(!side){//black
					if(can_move(i, 0, 0))moves->push_back(Move(i,0, i + delta_dir(0), false, NULL));
					if(can_move(i, 1, 0))moves->push_back(Move(i,0, i + delta_dir(1), false, NULL));
				}else{//white
					if(can_move(i, 2, 0))moves->push_back(Move(i,0, i + delta_dir(2), false, NULL));
					if(can_move(i, 3, 0))moves->push_back(Move(i,0, i + delta_dir(3), false, NULL));
				}
			}
		}
		return moves;
	}

	bool make_move(Move* move){
		assert(move != NULL);
		if(!move->jump){
			assert(t[move->dst] == EMPTY);
			Board::tile temp = t[move->pos];
			t[move->pos] = EMPTY;
			t[move->dst] = temp;
		}else{
			Move* next=move;
			//while(true){
				assert(t[move->dst] == EMPTY);
				Board::tile temp = t[move->pos];
				t[move->pos] = EMPTY;
				t[move->over] = EMPTY;
				t[move->dst] = temp;
				if(move->seq != NULL){
					next = move->seq;
				}else{
					//break;
					//TODO: jumping more than one stone
				}
			//}
		}
		if(t[move->dst]<(tile)QBLACK)//dont promote queens
			if(move->dst / 8 % 7 == 0)t[move->dst] = (tile)(t[move->dst]+2);
		return move->seq != NULL;
	}
};

int main(){
	std::random_device rd;
	std::mt19937 g(rd());
	Board b = Board(8);
	bool side = false;
	while(true){
		b.print();
		vector<Board::Move>* m = b.generate_moves(side);

		for(Board::Move& move : *m){
			printf("Move from %s to %s, jump: %d\n", b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
		}
		shuffle(m->begin(),m->end(),g);
		b.make_move(m->data());
		side = !side;
		delete m;
		printf("\n\n");
		char c = getc(stdin);
		if(c=='q')break;
	}
}
