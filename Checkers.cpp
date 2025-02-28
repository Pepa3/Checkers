#include <algorithm>
#include <iostream>
#include <cassert>
#include <variant>
#include <chrono>
#include <random>
#include <string>
#include <vector>

using namespace std;
constexpr int minimax_depth = 2*3;
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
		State(State& other){
			this->size = other.size;
			this->t = new tile[size];
			for(int i = 0; i < size; i++){
				t[i] = other.t[i];
			}
		}
		State(int size):size(size){
			t = new tile[size];
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
		~Move(){
			if(seq!=NULL)
			delete seq;
		}
		Move(const Move& other):jump(other.jump),pos(other.pos),over(other.over),dst(other.dst){
			if(other.seq!=NULL)
				seq = new Move(*other.seq);
			else seq=NULL;
		}
		Move(Move&& other):jump(other.jump),pos(other.pos),over(other.over),dst(other.dst){
			if(other.seq!=NULL)
				seq = new Move(*other.seq);
			else seq=NULL;
		}
		Move& operator=(const Move& other){
			jump = other.jump;
			pos = other.pos;
			over = other.over;
			dst = other.dst;
			if(other.seq!=NULL)
				seq = new Move(*other.seq);
			else seq=NULL;
			return *this;
		}
		Move() :jump(true), pos(0), over(0), dst(0), seq(NULL){}
		Move(uint8_t pos, uint8_t over, uint8_t dst, bool jump, Move* seq = NULL):jump(jump), pos(pos), over(over), dst(dst), seq(seq){}
		void print(){
			cout << "m[" << (pos % 8)+1 << ", " << (pos / 8)+1 << "]to[" << (dst % 8)+1 << ", " << (dst / 8)+1 << "]\n";
		}
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
	Board(Board& other){
		this->t = State(other.t);
		this->size = other.size;
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
			//cout << "Must jump" << endl;
			return generate_jumps(side,p);
		}
		return generate_slides(side);
	}
	inline string pretty_pos(int pos){
		return "["+to_string((pos%size)+1)+","+to_string((pos/size)+1)+"]";
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
			for(int i = 1; i <= max; i++){
				if(t[pos + diff * i] != EMPTY)return false;
			}
			return bounds(pos, diff, max) && (pos + diff * max) < (size * size);
		}
		assert(0 && "Unreachable");
		return false;
	}

	inline bool can_jump_s(bool side, int pos){
		if(!side){//black
			return (can_move(pos, 0, 1) && (t[pos + delta_dir(0)] == WHITE || t[pos + delta_dir(0)] == QWHITE))
				|| (can_move(pos, 1, 1) && (t[pos + delta_dir(1)] == WHITE || t[pos + delta_dir(1)] == QWHITE));
		} else{//white
			return (can_move(pos, 2, 1) && (t[pos + delta_dir(2)] == BLACK || t[pos + delta_dir(2)] == QBLACK))
				|| (can_move(pos, 3, 1) && (t[pos + delta_dir(3)] == BLACK || t[pos + delta_dir(3)] == QBLACK));
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
			if((t[pos + diff * i] == (tile) (BLACK + !side) || t[pos + diff * i] == (tile) (QBLACK + !side))){
				if(t[pos + diff * (i + 1)] == EMPTY && bounds(pos, diff, i + 1)){
					return true;
				} else{
					return false;
				}
			}
		}
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

	std::vector<Move> gen_jump_s(int pos, int dir){
		if(can_move(pos, dir, 1)){
			int nside = dir <= 1;
			int diff = delta_dir(dir);
			if(t[pos + diff] != (tile) (BLACK + nside) && t[pos + diff] != (tile) (QBLACK + nside))return vector<Move>();
			Move x = Move(pos,pos+diff,pos+diff*2,true);
			vector<Move> seq0 = gen_jump_s(pos + diff * 2, nside?0:2);
			vector<Move> seq1 = gen_jump_s(pos + diff * 2, nside?1:3);
			vector<Move> out = vector<Move>();
			if(!seq0.empty() && !seq1.empty()){
				for(auto& s : seq0){
					x.seq=new Move(s);
					out.push_back(x);
				}
				for(auto& s : seq1){
					x.seq=new Move(s);
					out.push_back(x);
				}
			}else if(!seq0.empty()){
				for(auto& s : seq0){
					x.seq=new Move(s);
					out.push_back(x);
				}
			}else if(!seq1.empty()){
				for(auto& s : seq1){
					x.seq=new Move(s);
					out.push_back(x);
				}
			}else{
				out.push_back(x);
			}
			return out;
		}else{
			return vector<Move>();
		}
	}

	Move* gen_jump_q(int pos, int dir,vector<int> seq){
		int diff = delta_dir(dir);
		Move* m = new Move[size-2]();
		int k = 0;
		for(int i = 1; i < size; i++){
			if(t[pos + diff * i] != EMPTY && t[pos + diff * i] != COUNT && find(seq.begin(), seq.end(), (pos+diff*i)) == seq.end()){
				for(int j = 0; j < size - 2; j++){
					m[j] = Move(pos, pos + diff * i, 0, false, NULL);
				}
				for(++i; i < size; i++){
					if(t[pos + diff * i] == EMPTY && bounds(pos, diff, i)){
						m[k].dst = pos + diff * i;
						m[k].jump = true;
						for(int dir2 = 0; dir2 < 4; dir2++){
							if(dir2!=((dir+2)%4) && can_jump_q((bool)(t[pos]-QBLACK), pos+diff*i, dir2)){
								seq.push_back(m[k].over);
								Move* x = gen_jump_q(pos+diff*i, dir2,seq);//stack overflow
								if(x != NULL){
									m[k].seq = x;
								}
							}
						}
						k++;
					} else{
						return m;
					}
				}
				return m;
			}
		}
		return NULL;//reachable if infinite sequence of queen jumps
	}

	vector<Move>* generate_jumps(bool side, int type){
		//assume jump is possible
		//queen jumps first, then stones, length ignored but the whole sequence
		vector<Move>* movesQ = new vector<Move>();
		vector<Move>* movesS = new vector<Move>();
		for(int i = 0; i < size * size; i++){
			Move* x;
			switch (t[i]){
			case QBLACK:
				if(!side){
					for(int dir = 0; dir < 4; dir++){
						if(can_jump_q(side, i, dir)){
							x = gen_jump_q(i, dir, vector<int>());
							if(x != NULL){
								for(int j = 0; j < size - 2; j++){
									if(x[j].jump) movesQ->push_back(x[j]);
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
							x = gen_jump_q(i, dir,vector<int>());
							if(x != NULL){
								for(int j = 0; j < size - 2; j++){
									if(x[j].jump) movesQ->push_back(x[j]);
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
						for(auto& s : gen_jump_s(i, 0)){
							movesS->push_back(s);
						}
						/*x = gen_jump_s(i, 0);
						if(x != NULL){
							movesS->push_back(*x);
							delete x;
						}*/
					}
					if(t[i + delta_dir(1)] == WHITE || t[i + delta_dir(1)] == QWHITE){
						for(auto& s : gen_jump_s(i, 1)){
							movesS->push_back(s);
						}
/*						x = gen_jump_s(i, 1);
						if(x != NULL){
							movesS->push_back(*x);
							delete x;
						}*/
					}
				}
				break;
			case WHITE:
				if(side && type != 2){
					if(t[i + delta_dir(2)] == BLACK || t[i + delta_dir(2)] == QBLACK){
						for(auto& s : gen_jump_s(i, 2)){
							movesS->push_back(s);
						}
/*						x = gen_jump_s(i, 2);
						if(x != NULL){
							movesS->push_back(*x);
							delete x;
						}*/
					}
					if(t[i + delta_dir(3)] == BLACK || t[i + delta_dir(3)] == QBLACK){
						for(auto& s : gen_jump_s(i, 3)){
							movesS->push_back(s);
						}
/*						x = gen_jump_s(i, 3);
						if(x != NULL){
							movesS->push_back(*x);
							delete x;
						}*/
					}
				}
				break;
			case EMPTY:
				break;
			default:
				assert(0 && "Unreachable");
			}
		}
		if(movesQ->size() == 0){
			delete movesQ;
			return movesS;
		} else{
			delete movesS;
			return movesQ;
		}
	}

	vector<Move>* generate_slides(bool side){
		vector<Move>* moves = new vector<Move>();
		for(int i = 0; i < size * size; i++){
			if(t[i] == (tile) (QBLACK + side)){
				for(int dir = 0; dir < 4; dir++){
					for(int j = 1; j < size; j++){
						int diff = delta_dir(dir);
						if((j == 1 && t[i + diff] == EMPTY && bounds(i,diff,1)) || (j != 1 && can_move(i, dir, max(2, j)))){
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

	bool make_move(const Move* move){
		if(move == NULL){
			cout << "No moves left." << endl;
			return false;
		}
		if(!move->jump){
			if(t[move->dst] != EMPTY){
				cout << "! move->dst != EMPTY" << endl;
				return false;
			}
			Board::tile temp = t[move->pos];
			t[move->pos] = EMPTY;
			t[move->dst] = temp;
		}else{
			const Move* next = move;
			while(true){
				if(t[next->dst] != EMPTY){
					cout << "! move->dst != EMPTY" << pretty_pos(next->pos)<< ">" << pretty_pos(next->dst) << endl;
					return false;
				}
				Board::tile temp = t[next->pos];
				t[next->pos] = EMPTY;
				t[next->over] = EMPTY;
				t[next->dst] = temp;
				if(next->seq != NULL && next->seq->jump==true){
					next = next->seq;
				}else{
					break;
				}
			}
		}
		for(int i = 0; i < size; i++){
			if(t[i] == BLACK){
				t[i] = (tile) (t[i] + 2);
			}
			if(t[size * size - 1 - i] == WHITE){
				t[size * size - 1 - i] = (tile) (t[size * size - 1 - i] + 2);
			}
		}
		return move->seq != NULL;
	}
	double eval(){
		double sb = 1;
		double sw = 1;
		for(int i = 0; i < size*size; i++){
			switch(t[i]){
			case BLACK:
				sb += 1+i/size;
				break;
			case WHITE:
				sw += 1+(size-i/size-1);
				break;
			case QBLACK:
				sb += 10.0;
				break;
			case QWHITE:
				sw += 10.0;
				break;
			default:
				break;
			}
		}
		return (sb-sw)/(sb+sw);
	}
};

std::variant<double,Board::Move*> minimax(Board& b, int depth, double alpha, double beta, bool side){
	if(depth == .0)return b.eval();
	Board::Move* best = NULL;
	if(!side){
		double maxEval = -1000.0;
		auto* m = b.generate_moves(side);
		for(const auto& m1 : *m){
			Board b1 = Board(b);
			b1.make_move(&m1);
			double e = get<double>(minimax(b1, depth - 1, alpha, beta, !side));
			if(depth == minimax_depth){
				printf("%f) Move from %s to %s, jump: %d\n", e, b.pretty_pos(m1.pos).c_str(), b.pretty_pos(m1.dst).c_str(), m1.jump);
			}
			maxEval = max(maxEval, e);
			alpha = max(alpha, e);
			//if(beta <= alpha)break;
			if(depth == minimax_depth && e >= maxEval){
				if(best != NULL)delete best;
				best = new Board::Move(m1);
			}
		}
		delete m;
		if(depth == minimax_depth){
			return best;
		} else{
			return maxEval;
		}
	} else{
		double minEval = 1000.0;
		auto* m = b.generate_moves(side);
		for(auto& m1 : *m){
			Board b1 = Board(b);
			b1.make_move(&m1);
			double e = get<double>(minimax(b1, depth - 1, alpha, beta, !side));
			if(depth == minimax_depth){
				printf("%f) Move from %s to %s, jump: %d\n", e, b.pretty_pos(m1.pos).c_str(), b.pretty_pos(m1.dst).c_str(), m1.jump);
			}
			minEval = min(minEval, e);
			beta = min(beta, e);
			//if(beta <= alpha)break;//doesnt free moves
			if(depth == minimax_depth && e <= minEval){
				if(best != NULL)delete best;
				best = new Board::Move(m1);
			}
		}
		delete m;
		if(depth == minimax_depth){
			return best;
		} else{
			return minEval;
		}
	}
}

int main(){
	std::random_device rd;
	std::mt19937 g(rd());
	Board b = Board(8);
	bool side = false;
	bool quit = false;
	while(!quit){
		vector<Board::Move>* m = b.generate_moves(side);

		for(size_t i = 0; i < m->size();i++){
			Board::Move& move = m->at(i);
			printf("%ld) Move from %s to %s, jump: %d\n", i, b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
		}
		b.print();
		printf("Please select move(0-%ld)\n", m->size()-1);
		string a = "";
		cin >> a;
		if(a == "q"){
			quit = true;
		}else if(a == "b"){
			const auto t1 = chrono::high_resolution_clock::now();
			Board::Move* best = get<Board::Move*>(minimax(b, minimax_depth, -1000, 1000, side));
			const auto t2 = chrono::high_resolution_clock::now();
			std::chrono::duration t = t2 - t1;
			cout << "Took " << t.count()/1000000 << "ms" << endl;
			if(best == NULL){
				cerr << "No move!\n";
				break;
			}
			best->print();
			b.make_move(best);
			delete best;
		}else{
			int s = std::stoi(a);
			b.make_move(&m->at(s));
		}
		//shuffle(m->begin(),m->end(),g);
		b.print();
		side = !side;
		/*for(const Board::Move& move : *m){
			if(move.seq!=NULL)delete[] move.seq;//seq.seq lost
		}*/
		delete m;
	}
}
