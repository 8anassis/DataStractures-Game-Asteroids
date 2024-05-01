
#include <stdlib.h>

#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "vec2.h"


// Οι ολοκληρωμένες πληροφορίες της κατάστασης του παιχνιδιού.
// Ο τύπος State είναι pointer σε αυτό το struct, αλλά το ίδιο το struct
// δεν είναι ορατό στον χρήστη.

struct state {
	Vector objects;			// περιέχει στοιχεία Object (αστεροειδείς, σφαίρες)
	struct state_info info;	// Γενικές πληροφορίες για την κατάσταση του παιχνιδιού
	int next_bullet;		// Αριθμός frames μέχρι να επιτραπεί ξανά σφαίρα
	float speed_factor;		// Πολλαπλασιαστής ταχύτητς (1 = κανονική ταχύτητα, 2 = διπλάσια, κλπ)
};


// Δημιουργεί και επιστρέφει ένα αντικείμενο

static Object create_object(ObjectType type, Vector2 position, Vector2 speed, Vector2 orientation, double size) {
	Object obj = malloc(sizeof(*obj));
	obj->type = type;
	obj->position = position;
	obj->speed = speed;
	obj->orientation = orientation;
	obj->size = size;
	return obj;
}

// Επιστρέφει έναν τυχαίο πραγματικό αριθμό στο διάστημα [min,max]

static double randf(double min, double max) {
	return min + (double)rand() / RAND_MAX * (max - min);
}

// Προσθέτει num αστεροειδείς στην πίστα (η οποία μπορεί να περιέχει ήδη αντικείμενα).
//
// ΠΡΟΣΟΧΗ: όλα τα αντικείμενα έχουν συντεταγμένες x,y σε ένα καρτεσιανό επίπεδο.
// - Η αρχή των αξόνων είναι η θέση του διαστημόπλοιου στην αρχή του παιχνιδιού
// - Στο άξονα x οι συντεταγμένες μεγαλώνουν προς τα δεξιά.
// - Στον άξονα y οι συντεταγμένες μεγαλώνουν προς τα πάνω.

static void add_asteroids(State state, int num) {
	for (int i = 0; i < num; i++) {
		// Τυχαία θέση σε απόσταση [ASTEROID_MIN_DIST, ASTEROID_MAX_DIST]
		// από το διστημόπλοιο.
		//
		Vector2 position = vec2_add(
			state->info.spaceship->position,
			vec2_from_polar(
				randf(ASTEROID_MIN_DIST, ASTEROID_MAX_DIST),	// απόσταση
				randf(0, 2*PI)									// κατεύθυνση
			)
		);

		// Τυχαία ταχύτητα στο διάστημα [ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED]
		// με τυχαία κατεύθυνση.
		//
		Vector2 speed = vec2_from_polar(
			randf(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED) * state->speed_factor,
			randf(0, 2*PI)
		);

		Object asteroid = create_object(
			ASTEROID,
			position,
			speed,
			(Vector2){0, 0},								// δεν χρησιμοποιείται για αστεροειδείς
			randf(ASTEROID_MIN_SIZE, ASTEROID_MAX_SIZE)		// τυχαίο μέγεθος
		);
		vector_insert_last(state->objects, asteroid);
	}
}

// Δημιουργεί και επιστρέφει την αρχική κατάσταση του παιχνιδιού

State state_create() {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	// Γενικές πληροφορίες
	state->info.paused = false;				// Το παιχνίδι ξεκινάει χωρίς να είναι paused.
	state->speed_factor = 1;				// Κανονική ταχύτητα
	state->next_bullet = 0;					// Σφαίρα επιτρέπεται αμέσως
	state->info.score = 0;				// Αρχικό σκορ 0

	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	state->objects = vector_create(0, NULL);

	// Δημιουργούμε το διαστημόπλοιο
	state->info.spaceship = create_object(
		SPACESHIP,
		(Vector2){0, 0},			// αρχική θέση στην αρχή των αξόνων
		(Vector2){0, 0},			// μηδενική αρχική ταχύτητα
		(Vector2){0, 1},			// κοιτάει προς τα πάνω
		SPACESHIP_SIZE				// μέγεθος
	);

	// Προσθήκη αρχικών αστεροειδών
	add_asteroids(state, ASTEROID_NUM);

	return state;
}

// Επιστρέφει τις βασικές πληροφορίες του παιχνιδιού στην κατάσταση state

StateInfo state_info(State state) {
	return &state->info;
}

// Επιστρέφει μια λίστα με όλα τα αντικείμενα του παιχνιδιού στην κατάσταση state,
// των οποίων η θέση position βρίσκεται εντός του παραλληλογράμμου με πάνω αριστερή
// γωνία top_left και κάτω δεξιά bottom_right.

List state_objects(State state, Vector2 top_left, Vector2 bottom_right) {
	List res = list_create(NULL);
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object obj = vector_get_at(state->objects, i);
		if (&obj->position <= &top_left && &obj->position <= &bottom_right)
			list_insert_next(res, LIST_BOF, obj);
	}
	return res;
}

// Ενημερώνει την κατάσταση state του παιχνιδιού μετά την πάροδο 1 frame.
void state_update(State state, KeyState keys) {
	
	// Κίνηση των ASTEROID και BULLET
	for ( int i = 0; i < vector_size(state->objects); i++ ){
		Object obj = vector_get_at(state->objects, i);

		obj->position = vec2_add(obj->position, obj->speed);
	}

	// Πλοήγηση διαστημόπλοιου.
	if (keys->left){
		state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, SPACESHIP_ROTATION);
	}

	if (keys->right){
		state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, -SPACESHIP_ROTATION);
	}

	if (keys->up){
		state->info.spaceship->speed = vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, SPACESHIP_ACCELERATION));
	}

	if (!keys->up){
		float num = vec2_distance(state->info.spaceship->speed, (Vector2){0,0});
		if (num)
		state->info.spaceship->speed = vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, SPACESHIP_SLOWDOWN));
	}

	//Παύση και διακοπή:
    if (keys->p){
		state->info.paused = !state->info.paused;
		if (state->info.paused == true && keys->n){
			state_update(state, keys);
		}
	}

	int count_asteroid = 0;
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object obj = vector_get_at(state->objects, i);
				
		if (obj->type == ASTEROID){
			if (vec2_distance(obj->position, state->info.spaceship->position) < 400){
				count_asteroid += ASTEROID;
				if (count_asteroid < ASTEROID_NUM){   
				add_asteroids(state, ASTEROID_NUM - count_asteroid); // Διατήρησε τους ASTEROID στους 6, "κοντά" στο spaceship.
				}
			}
			if ( CheckCollisionCircles(state->info.spaceship->position, state->info.spaceship->size /2, obj->position, obj->size /2)){
			free(obj);
			state->info.score = state->info.score / 2;
			}
		}
	}

	state->next_bullet += 1;
	if (keys->space){
		if (state->next_bullet > 15){
			state->next_bullet = 0;
			Object bullet = create_object(BULLET, state->info.spaceship->position,
			vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, BULLET_SPEED)),
			state->info.spaceship->orientation, BULLET_SIZE);

			vector_insert_last(state->objects, bullet);
			
		}

	}
}

// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη.

void state_destroy(State state) {
	// Προς υλοποίηση
}
