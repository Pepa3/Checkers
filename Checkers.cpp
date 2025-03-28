#include <algorithm>
#include <iostream>
#include <cassert>
#include <variant>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using namespace std;
constexpr int minimax_depth = 2*3+1;
const SDL_Color black = SDL_Color{0x33,0x33,0x33,0xff};
const SDL_Color white = SDL_Color{0xbb,0xbb,0xbb,0xff};
const SDL_Color gray =  SDL_Color{0x77,0x77,0x77,0xff};
const SDL_Color brown = SDL_Color{0x96,0x4b,0x00,0xff};

class Board{
public:
	enum tile : uint8_t{ EMPTY = 0, BLACK, WHITE, QBLACK, QWHITE, COUNT };
	class State{
	public:
		tile t[64];
		struct _State{
			State* s;
			int j;
		public:
			_State(State* s, int j) :s(s), j(j){}
			inline char operator()()const{ assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT); return " xoXO+"[s->t[j]]; }
			inline _State operator= (tile v){
				assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT);
				s->t[j] = v;
				return *this;
			}
			inline operator tile(){
				assert(j >= 0 && j < 8 * 8); assert(s->t[j] >= 0 && s->t[j] <= COUNT);
				return s->t[j];
			}
			inline bool operator== (tile v){ if(j >= 0 && j < 8*8)return s->t[j] == v; else return v == COUNT; }
			inline bool operator!= (tile v){ if(j >= 0 && j < 8*8)return s->t[j] != v; else return v != COUNT; }
		};
		State(const State& other){
			for(int i = 0; i < 8*8; i++){
				t[i] = other.t[i];
			}
		}
		State(){
			for(int i = 0; i < 8*8; i++){
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
			if(seq!=NULL){
				delete seq;
			}
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
	State t = State();
	Board(){
		for(int i = 0; i < 8*2; i++){
			if((i+i/8) % 2 == 1){
				t[i] = WHITE;
				t[8 * 8 - i - 1] = BLACK;
			}
		}
	}
	Board(const Board& other){
		this->t = other.t;
	}
	void print(){
		for(int i = 0; i < 8; i++){
			cout << "+-+-+-+-+-+-+-+-+\n";
			for(int j = 0; j < 8; j++){
				cout << "|" << t[j + i * 8]();
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
	inline static string pretty_pos(int pos){
		return "["+to_string((pos%8)+1)+","+to_string((pos/8)+1)+"]";
	}
	inline int delta_dir(const int dir)const{
		//0=^\ 1=/^ 2=\v 3=v/
		assert(dir >= 0 && dir < 4);
		return (dir % 3 == 0 ? -1 : 1) + (8 * (dir < 2 ? -1 : 1));
	}
	inline bool bounds(int pos, int diff, int dist)const{
		return (abs(pos % 8 - (pos + diff*dist) % 8) == dist && abs(pos / 8 - (pos + diff*dist) / 8) == dist);
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
			return bounds(pos, diff, max) && (pos + diff * max) < (8 * 8);
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
		for(int i = 1; i < 8; i++){
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
		for(int i = 0; i < 8 * 8; i++){
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

	std::vector<Move> gen_jump_q(int pos, int dir, vector<int> seq = vector<int>()){
		int diff = delta_dir(dir);
		std::vector<Move> m = std::vector<Move>();
		std::vector<Move> mS = std::vector<Move>();
		int k = 0;
		bool side = (bool)(t[pos] - QBLACK);
		for(int i = 1; i < 8; i++){
			if((t[pos + diff * i] == (tile)(BLACK+!side) || t[pos + diff * i] == (tile) (QBLACK + !side))&& t[pos + diff * i] != COUNT && find(seq.begin(), seq.end(), (pos+diff*i)) == seq.end()){
				Move x = Move(pos, pos + diff * i, 0, false);
				for(++i; i < 8; i++){
					if(t[pos + diff * i] == EMPTY && bounds(pos, diff, i)){
						x.dst = pos + diff * i;
						x.jump = true;
						m.push_back(x);
						for(int dir2 = 0; dir2 < 4; dir2++){
							if(dir2!=((dir+2)%4) && can_jump_q((bool)(t[pos]-QBLACK), pos+diff*i, dir2)){
								seq.push_back(x.over);
								std::vector<Move> s = gen_jump_q(pos+diff*i, dir2,seq);
								if(!s.empty()){
									for(auto& s1 : s){
										x.seq=new Move(s1);
										mS.push_back(x);
									}
								}
							}
						}
						k++;
					} else{
						if(mS.empty())
							return m;
						else
							return mS;
					}
				}
				if(mS.empty())
					return m;
				else
					return mS;
			}
		}
		return vector<Move>();//reachable if infinite sequence of queen jumps
	}

	vector<Move>* generate_jumps(bool side, int type){
		//assume jump is possible
		//queen jumps first, then stones, length ignored but the whole sequence
		vector<Move>* movesQ = new vector<Move>();
		vector<Move>* movesS = new vector<Move>();
		for(int i = 0; i < 8 * 8; i++){
			switch (t[i]){
			case QBLACK:
				if(!side){
					for(int dir = 0; dir < 4; dir++){
						if(can_jump_q(side, i, dir)){
							for(auto& s : gen_jump_q(i, dir)){
								movesQ->push_back(s);
							}
						}
					}
				}
				break;
			case QWHITE:
				if(side){
					for(int dir = 0; dir < 4; dir++){
						if(can_jump_q(side, i, dir)){
							for(auto& s : gen_jump_q(i, dir)){
								movesQ->push_back(s);
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
					}
					if(t[i + delta_dir(1)] == WHITE || t[i + delta_dir(1)] == QWHITE){
						for(auto& s : gen_jump_s(i, 1)){
							movesS->push_back(s);
						}
					}
				}
				break;
			case WHITE:
				if(side && type != 2){
					if(t[i + delta_dir(2)] == BLACK || t[i + delta_dir(2)] == QBLACK){
						for(auto& s : gen_jump_s(i, 2)){
							movesS->push_back(s);
						}
					}
					if(t[i + delta_dir(3)] == BLACK || t[i + delta_dir(3)] == QBLACK){
						for(auto& s : gen_jump_s(i, 3)){
							movesS->push_back(s);
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
		for(int i = 0; i < 8 * 8; i++){
			if(t[i] == (tile) (QBLACK + side)){
				for(int dir = 0; dir < 4; dir++){
					for(int j = 1; j < 8; j++){
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
				//cout << "move" << pretty_pos(next->pos) << ">" << pretty_pos(next->dst) << "\n";
				if(t[next->dst] != EMPTY){
					cout << "! move->dst != EMPTY" << pretty_pos(next->pos)<< ">" << pretty_pos(next->dst) << endl;
					return false;
				}
				Board::tile temp = t[next->pos];
				t[next->pos] = EMPTY;
				t[next->over] = EMPTY;
				t[next->dst] = temp;
				if(next->seq != NULL){
					if(next->seq->jump==true){
						next = next->seq;
						//cout << "Jumping to " << pretty_pos(next->dst)<<endl;
					}
				}else{
					break;
				}
			}
		}
		for(int i = 0; i < 8; i++){
			if(t[i] == BLACK){
				t[i] = (tile) (t[i] + 2);
			}
			if(t[8 * 8 - 1 - i] == WHITE){
				t[8 * 8 - 1 - i] = (tile) (t[8 * 8 - 1 - i] + 2);
			}
		}
		return move->seq != NULL;
	}
	double eval(){
		double sb = 1;
		double sw = 1;
		for(int i = 0; i < 8*8; i++){
			switch(t[i]){
			case BLACK:
				sb += 10+i/8;
				break;
			case WHITE:
				sw += 10+(8-i/8-1);
				break;
			case QBLACK:
				sb += 100.0;
				break;
			case QWHITE:
				sw += 100.0;
				break;
			default:
				break;
			}
		}
		int m = 1;
		if(sb == 1)return m*-1000;
		if(sw == 1)return m*1000;
		return m*(sb-sw)/(sb+sw);
	}
};

std::variant<double,Board::Move> minimax(Board b, int depth, double alpha, double beta, bool side){
	if(depth == 0)return b.eval();
	Board::Move best;
	if(!side){
		double maxEval = -1000.0;
		vector<Board::Move>* m = b.generate_moves(side);
		if(m->size()==1 && depth==minimax_depth){
			return m->at(0);
		}
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
				best = m1;
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
		vector<Board::Move>* m = b.generate_moves(side);
		if(m->size()==1 && depth==minimax_depth){
			return m->at(0);
		}
		for(const auto& m1 : *m){
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
				best = m1;
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

void draw_circle(SDL_Renderer* renderer, int x, int y, int radius, SDL_Color color){
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	for(int w = 0; w < radius * 2; w++){
		for(int h = 0; h < radius * 2; h++){
			int dx = radius - w; // horizontal offset
			int dy = radius - h; // vertical offset
			if((dx * dx + dy * dy) <= (radius * radius)){
				SDL_RenderPoint(renderer, x + dx, y + dy);
			}
		}
	}
}

int main(int argc, char** argv){
	//TODO:convert into main callbacks
	if(!SDL_SetAppMetadata("Checkers", "1.0", "me.pepa3.checkers")){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "metadata: %s", SDL_GetError());
	}
	SDL_Window* window;
	SDL_Renderer* render;
	bool done = false;
	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_CAMERA)){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL context: %s", SDL_GetError());
		return 1;
	}
	if(!SDL_CreateWindowAndRenderer("Checkers", 800, 800, 0, &window, &render)){
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,"Couldn't create window/renderer: %s", SDL_GetError());
		return 1;
	}
	SDL_SetRenderVSync(render, SDL_RENDERER_VSYNC_ADAPTIVE);
	SDL_FRect rect = {0,0,100,100};
	vector<Board::Move>* m = NULL;
	vector<Board::Move> m1 = vector<Board::Move>();
	vector<Board::Move> hist = vector<Board::Move>();
	bool selected = false;
	int seqDepth = 0;
	Board b = Board();
	bool side = false;
	m = b.generate_moves(side);

	for(size_t i = 0; i < m->size(); i++){
		Board::Move& move = m->at(i);
		printf("%ld) Move from %s to %s, jump: %d\n", i, b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
	}
	m1 = *m;
	b.print();
	while(!done){
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_EVENT_QUIT){
				done = true;
			} else if(event.type == SDL_EVENT_KEY_DOWN){
				char key = event.key.key;
				if(key == SDLK_ESCAPE){
					for(size_t i = 0; i < hist.size(); i++){
						Board::Move& move = hist.at(i);
						printf("%ld) Move from %s to %s, jump: %d\n", i, b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
						while(move.seq != NULL){
							move = *move.seq;
							cout << "to " << b.pretty_pos(move.dst) << endl;
						}
					}
					exit(0);
				} else if(key == SDLK_B){
					const auto t1 = chrono::high_resolution_clock::now();
					Board::Move best = get<Board::Move>(minimax(b, minimax_depth, -1000, 1000, side));
					const auto t2 = chrono::high_resolution_clock::now();
					std::chrono::duration t = t2 - t1;
					cout << "Took " << t.count() / 1000000 << "ms" << endl;
					best.print();
					b.make_move(&best);
					hist.push_back(best);
					side = !side;
					delete m;
					m = b.generate_moves(side);
					m1 = *m;
					for(size_t i = 0; i < m->size(); i++){
						Board::Move& move = m->at(i);
						printf("%ld) Move from %s to %s, jump: %d\n", i, b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
					}

					b.print();
					selected = false;
				}
			} else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
				if(m == NULL){
					m = b.generate_moves(side);
					m1 = *m;
				}
				if(event.button.button == SDL_BUTTON_LEFT){
					int x = event.button.x / 100;
					int y = event.button.y / 100;
					if(!selected){
						erase_if(m1, [=](const Board::Move& c){return c.pos!=x+y*8; });
						for(size_t i = 0; i < m1.size(); i++){
							Board::Move& move = m1.at(i);
							printf("%ld) Move from %s to %s, jump: %d\n", i, b.pretty_pos(move.pos).c_str(), b.pretty_pos(move.dst).c_str(), move.jump);
						}
						selected = true;
						seqDepth = 0;
					}else{
						erase_if(m1, [=](const Board::Move& c){
							if(seqDepth==0)	return c.dst != x + y * 8; 
							else{
								const Board::Move* move = &c;
								for(int i = 0; i < seqDepth; i++){
									move = move->seq;
								}
								return move->dst == x + y * 8;
							}
						});
						seqDepth++;
					}
					if(m1.size() == 1){
						b.make_move(&m1.at(0));
						hist.push_back(m1.at(0));
						side = !side;
						delete m;
						m = NULL;
						m1.clear();
						seqDepth = 0;
					}
				} else if(event.button.button==SDL_BUTTON_RIGHT){
					m1 = *m;
					selected = false;
					seqDepth = 0;
				}
			}
		}
		SDL_SetRenderDrawColor(render, gray.r, gray.g, gray.b, gray.a);
		SDL_RenderClear(render);
		for(int i = 0; i < 64; i++){
			rect.x = 100 * (i % 8);
			rect.y = 100 * (i / 8);
			SDL_Color x = ((i % 8 + i / 8) % 2 == 0 ? white : black);
			SDL_SetRenderDrawColor(render, x.r, x.g, x.b, x.a);
			if(!selected && find_if(m1.begin(), m1.end(), [=](const Board::Move& c){return c.pos == i; }) != m1.end()){
				SDL_SetRenderDrawColor(render, 0, 0xff, 0, 0xff);//green
			} else if(selected && find_if(m1.begin(), m1.end(), [=](const Board::Move& c){return c.dst == i; }) != m1.end()){
				SDL_SetRenderDrawColor(render, 0, 0xff, 0, 0xff);//green
			}
			SDL_RenderFillRect(render, &rect);
			Board::tile t = b.t[i];
			if(t != Board::tile::EMPTY){
				bool pside = (t - Board::BLACK) < 2 ? (bool) (t - Board::BLACK) : (bool) (t - Board::QBLACK);
				draw_circle(render, 100 * (i % 8) + 50, 100 * (i / 8) + 50, 35, pside ? SDL_Color{0xff,0xff,0xff,0xff} : brown);
				if(t >= Board::QBLACK){
					draw_circle(render, 100 * (i % 8) + 50, 100 * (i / 8) + 50, 15, SDL_Color{0xff,0xff,0x00,0xff});
				}
			}
		}
		SDL_RenderPresent(render);
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
