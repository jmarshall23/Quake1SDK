/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "r_local.h"

#define MAX_PARTICLES			2048	// default max # of particles at one
										//  time
#define ABSOLUTE_MIN_PARTICLES	512		// no fewer than this no matter what's
										//  on the command line

int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

particle_t	*active_particles, *free_particles;

particle_t	*particles;
int			r_numparticles;

vec3_t			r_pright, r_pup, r_ppn;


/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
	else
	{
		r_numparticles = MAX_PARTICLES;
	}

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");
}

#ifdef QUAKE2
void R_DarkFieldParticles (entity_t *ent)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	org[0] = ent->origin[0];
	org[1] = ent->origin[1];
	org[2] = ent->origin[2];
	for (i=-16 ; i<16 ; i+=8)
		for (j=-16 ; j<16 ; j+=8)
			for (k=0 ; k<32 ; k+=8)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 150 + rand()%6;
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}
#endif


/*
===============
R_EntityParticles
===============
*/

#define NUMVERTEXNORMALS	162
extern	float	r_avertexnormals[NUMVERTEXNORMALS][3];
vec3_t	avelocities[NUMVERTEXNORMALS];
float	beamlength = 16;
vec3_t	avelocity = {23, 7, 3};
float	partstep = 0.01;
float	timescale = 0.01;

void R_EntityParticles (entity_t *ent)
{
	int			count;
	int			i;
	particle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist;
	
	dist = 64;
	count = 50;

if (!avelocities[0][0])
{
for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
avelocities[0][i] = (rand()&255) * 0.01;
}


	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = cl.time * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->start = cl.time;

		p->die = cl.time + 0.01;
		p->color = 0x6f;
		p->type = pt_explode;
		
		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;			
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;			
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;			
	}
}


/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<r_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles-1].next = NULL;
}


void R_ReadPointFile_f (void)
{
	FILE	*f;
	vec3_t	org;
	int		r;
	int		c;
	particle_t	*p;
	char	name[MAX_OSPATH];
	
	sprintf (name,"maps/%s.pts", sv.name);

	COM_FOpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		if (!free_particles)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->start = cl.time;
		p->die = 99999;
		p->color = (-c)&15;
		p->type = pt_static;
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

if (msgcount == 255)
	count = 1024;
else
	count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
}
	
/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion (const vec3_t & org)
{
	int i, j;
	particle_t* p;

	// Increase the number of particles generated for a denser explosion
	for (i = 0; i < 2048; i++) // Increased from 1024 to 2048
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->start = cl.time;

		// Adjust die time for more variation in particle lifespan
		p->die = cl.time + (3 + (rand() % 5)); // Now varies from 3 to 7

		// Introduce more color variations
		p->color = ramp1[rand() % 5]; // Assuming ramp1 has at least 5 different colors

		// Introduce a random size multiplier, affecting how large each particle appears
		p->size = 1.0f + (float)(rand() % 100) / 100.0f; // Size varies from 1.0 to 2.0

		// More variation in the ramp to affect particle appearance over time
		p->ramp = rand() % 5; // Increased variability in appearance

		p->growth = 1.0f + ((rand() % 401) / 100.0f); // Random growth between 1.0 and 5.0

		if (i % 3 == 0) // Change condition to create three types of particles
		{
			p->type = pt_explode;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 64) - 32); // Increased spread
				p->vel[j] = (rand() % 1024) - 512; // Increased velocity range
			}
		}
		else if (i % 3 == 1)
		{
			p->type = pt_explode2;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 48) - 24); // Moderately increased spread
				p->vel[j] = (rand() % 768) - 384; // Moderately increased velocity range
			}
		}
		else // Adding a new particle type for additional variety
		{
			p->type = pt_explode3; // Assuming pt_explode3 is a new particle type defined elsewhere
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16); // Original spread
				p->vel[j] = (rand() % 512) - 256; // Original velocity range
			}
		}
	}
}

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2 (const vec3_t & org, int colorStart, int colorLength)
{
	int			i, j;
	particle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->start = cl.time;
		p->die = cl.time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%512)-256;
		}
	}
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion (const vec3_t & org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->start = cl.time;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 1 + (rand()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect (const vec3_t & org, const vec3_t & dir, int color, int count)
{
	int i, j;
	particle_t* p;

	// Blood
	if (color == 73) {
		count = 35 + (rand() % 75); // This will set count to a random number between 100 and 200
	}

	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		// Adjust the die time and color for more subtle effects
		p->start = cl.time;
		p->die = cl.time + 0.2 + (rand() % 50) / 100.0; // Lasts between 0.2 to 0.7 seconds
		p->color = (color & ~7) + (rand() & 7); // Slight variation in color
		p->ramp = (rand() & 3) + 2;
		p->growth = 1.0f + ((rand() % 401) / 100.0f); // Random growth between 1.0 and 5.0
		p->size = 0.5f + ((rand() % 101) / 100.0f); // Random size between 0.5 and 1.5

		// Blood
		if (color == 73) { 
			p->die = cl.time + 3 + (rand() % 201) / 100.0; // Lasts between 3 to 5 seconds
			p->color = color; // Use the exact color for blood without variation
			p->type = pt_blood;
			// Calculate a normalized inverse direction vector
			vec3_t inverseDir;
			for (j = 0; j < 3; j++) {
				inverseDir[j] = -dir[j];
			}
			VectorNormalize(inverseDir); // Assuming there's a function to normalize the vector

			for (j = 0; j < 3; j++) {
				// Adjust origin for concentrated offset as before
				p->org[j] = org[j] + ((rand() & 15) - 8);

				// Significantly increase the inverse direction velocity and randomness for a violent effect
				float angleSpread = 0.3; // Increase the spread of the cone for more violence
				float baseVelocity = 120; // Increase base velocity for faster movement
				float randomVelocityComponent = (rand() % 400 - 200) * angleSpread; // Increase randomness
				p->vel[j] = inverseDir[j] * baseVelocity + randomVelocityComponent;
			}
		}
		// Differentiate between high-impact (explosion) and low-impact (bullet hit) effects
		else if (count > 100) { // Assume higher counts correspond to larger explosions
			p->type = (i % 2 == 0) ? pt_explode : pt_explode2;
			for (j = 0; j < 3; j++) {
				p->org[j] = org[j] + ((rand() % 64) - 32); // Increased randomness in origin
				p->vel[j] = (rand() % 1024) - 512; // Increased velocity for a more dynamic effect
			}
		}
		else { // Smaller counts for bullet hits
			p->type = pt_smoke;
			for (j = 0; j < 3; j++) {
				p->org[j] = org[j] + ((rand() & 7) - 4); // Smaller, more concentrated origin offset
				p->vel[j] = dir[j] * 20 + ((rand() % 100) - 50); // Directional velocity with slight randomness
			}
			p->vel[2] += 25; // Additional upward velocity for smoke to rise
		}

		// For smoke effects, gradually decrease velocity to simulate air resistance
		if (p->type == pt_smoke) {
			for (j = 0; j < 3; j++) {
				p->vel[j] *= 0.9; // Apply a damping effect to simulate air resistance
			}
		}
	}
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (const vec3_t & org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->start = cl.time;
				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (const vec3_t & org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->start = cl.time;
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

void R_RocketTrail(vec3_t& start, const vec3_t& end, int type) {
	vec3_t vec, right, up;
	float len;
	int j;
	particle_t* p;
	int dec;
	static int tracercount;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	// Generate "right" and "up" vectors to create a cone around "vec"
	PerpendicularVector(right, vec); // Generate a vector perpendicular to "vec"
	CrossProduct(vec, right, up); // Cross product to get "up" vector, perpendicular to both "vec" and "right"
	float particleDensity = 0.5;

	if (type < 128) {
		dec = 1;
	}
	else {
		dec = 1;
		type -= 128;
	}

	while (len > 0) {
		len -= dec * particleDensity;

		if (!free_particles) return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		//VectorClear(p->vel);
		p->start = cl.time;
		p->vel = vec3_t(0, 0, 0);
		p->die = cl.time + (type == 0 ? 2.5 : 2);
		p->growth = 5.0f + ((rand() % 501) / 100.0f); // Random growth between 5.0 and 10.0
		p->size = 0.5f + ((rand() % 101) / 100.0f); // Random size between 0.5 and 1.5

		float coneSpread = 0.1; // Adjust for wider or narrower cone
		vec3_t randomDir;
		switch (type) {
			case 0: // Enhanced rocket trail with cone effect
			{
				float zSpreadMultiplier = 3.5; // Increase spread on the z-axis

				// Assume vec is normalized direction from start to end
				vec3_t inverseDir;
				VectorSubtract(start, end, inverseDir); // Get the inverse direction vector
				VectorNormalize(inverseDir); // Normalize it

				vec3_t right, up;
				// Generate perpendicular vectors "right" and "up" based on the inverse direction
				PerpendicularVector(right, inverseDir);
				CrossProduct(inverseDir, right, up);
				VectorNormalize(up); // Ensure up vector is normalized

				float coneSpread = 0.3; // Increase for a wider cone effect

				p->ramp = (rand() & 3) * 0.5f;
				p->color = ramp3[(int)p->ramp];
				p->type = pt_firetrail;
				p->size = 0.5f + ((rand() % 101) / 100.0f); // Random size between 0.5 and 1.5

				// Apply a bigger cone effect facing the inverse direction
				for (j = 0; j < 3; j++) {
					// Calculate randomDir within the cone spread in the inverse direction
					if (j != 2) { // x and y components
						randomDir[j] = inverseDir[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
					}
					else { // z component with increased spread
						randomDir[j] = inverseDir[j] + ((rand() % 201 - 100) / 1000.0f * coneSpread * zSpreadMultiplier) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread * zSpreadMultiplier);
					}
				}
				VectorNormalize(randomDir); // Ensure the direction vector is normalized

				for (j = 0; j < 3; j++) {
					p->org[j] = start[j] + randomDir[j] * ((rand() % 6) - 3);
					p->vel[j] = randomDir[j] * 25; // Increase velocity for a more dramatic effect
				}
			}
			break;

			case 1: // Smoke with cone effect
			{
				p->die = cl.time + 4 + (rand() % 7);
				p->ramp = (rand() & 3) + 2;
				p->color = (p->ramp == 4) ? 0 : ramp3[(int)p->ramp];
				p->type = pt_smoketrail;
				p->size = 0.5f + ((rand() % 101) / 100.0f); // Random size between 0.5 and 1.5

				float coneSpread = 0.5; // Existing coneSpread for x and y directions
				float zSpreadMultiplier = 1.5; // Increase spread on the z-axis

				// Apply cone effect for smoke with more z-axis spread
				for (j = 0; j < 3; j++) {
					if (j != 2) { // x and y components
						randomDir[j] = vec[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
					}
					else { // z component with increased spread
						randomDir[j] = vec[j] + ((rand() % 201 - 100) / 1000.0f * coneSpread * zSpreadMultiplier) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread * zSpreadMultiplier);
					}
				}
				VectorNormalize(randomDir);

				for (j = 0; j < 3; j++) {
					p->org[j] = start[j] + randomDir[j] * ((rand() % 8) - 4);
					// Apply slightly more velocity in the z-axis to match the increased spread
					if (j != 2) { // x and y components
						p->vel[j] = randomDir[j] * 60; // Existing velocity adjusted for smoke
					}
					else { // z component with slightly more velocity
						p->vel[j] = randomDir[j] * 60 * zSpreadMultiplier; // Match the spread with increased velocity
					}
				}
			}
			break;

			case 2: // Blood with cone effect
				p->type = pt_grav;
				p->color = 67 + (rand() & 3);
				// Apply cone effect for blood
				for (j = 0; j < 3; j++) {
					randomDir[j] = vec[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
				}
				VectorNormalize(randomDir);
				for (j = 0; j < 3; j++) {
					p->org[j] = start[j] + randomDir[j] * ((rand() % 6) - 3);
				}
				break;
			case 3: // Tracer 1 with cone effect
			case 5: // Tracer 2 with cone effect
				p->die = cl.time + 0.5;
				p->type = pt_static;
				p->color = (type == 3) ? 52 + ((tracercount & 4) << 1) : 230 + ((tracercount & 4) << 1);

				tracercount++;

				// Apply cone effect for tracer
				for (j = 0; j < 3; j++) {
					randomDir[j] = vec[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
				}
				VectorNormalize(randomDir);
				// Tracers have a more straightforward path, so the cone effect is subtly applied
				VectorCopy(start, p->org);
				for (j = 0; j < 3; j++) {
					p->vel[j] = randomDir[j] * 30; // Adjusted velocity for tracers
				}
				break;

			case 4: // Slight blood with cone effect
				p->type = pt_grav;
				p->color = 67 + (rand() & 3);
				// Apply cone effect for slight blood
				for (j = 0; j < 3; j++) {
					randomDir[j] = vec[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
				}
				VectorNormalize(randomDir);
				for (j = 0; j < 3; j++) {
					p->org[j] = start[j] + randomDir[j] * ((rand() % 6) - 3);
				}
				len -= 3; // Decrement len for slight blood, ensure particles spread out
				break;

			case 6: // Voor trail with cone effect
				p->color = 9 * 16 + 8 + (rand() & 3);
				p->type = pt_static;
				p->die = cl.time + 0.5; // Adjusted lifespan for more visible trail

				// Apply cone effect for Voor trail
				for (j = 0; j < 3; j++) {
					randomDir[j] = vec[j] + right[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread) + up[j] * ((rand() % 201 - 100) / 1000.0f * coneSpread);
				}
				VectorNormalize(randomDir);
				for (j = 0; j < 3; j++) {
					p->org[j] = start[j] + randomDir[j] * ((rand() & 15) - 8); // Enhanced spread for the Voor trail
					p->vel[j] = randomDir[j] * 10; // Adjusted velocity for the effect
				}
				break;
		}

		VectorAdd(start, vec, start); // Move start along the vector for the next particle
	}
}


/*
===============
R_DrawParticles
===============
*/
extern	cvar_t	sv_gravity;

void R_DrawParticles (void)
{
	particle_t		*p, *kill;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;
	
#ifdef GLQUAKE
	vec3_t			up, right;
	float			scale;

    GL_Bind(particletexture);
	glEnable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin (GL_TRIANGLES);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);
#else
	D_StartParticles ();

	VectorScale (vright, xscaleshrink, r_pright);
	VectorScale (vup, yscaleshrink, r_pup);
	VectorCopy (vpn, r_ppn);
#endif
	frametime = cl.time - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * sv_gravity.value * 0.05;
	dvel = 4*frametime;
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}


	

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

#ifdef GLQUAKE
		// Update particle size based on its growth rate and frametime
		p->size += p->growth * frametime;

		scale = p->size;

		// Ensure the particle size is within reasonable limits, if necessary
		// This is optional and depends on the desired effect
		if (p->size < 1.0f) p->size = 1.0f; // Minimum size
		//if (p->size > 10.0f) p->size = 10.0f; // Maximum size, adjust as needed

		//glColor3ubv ((byte *)&d_8to24table[(int)p->color]);
		// Convert d_8to24table color from 0-255 range to 0-1 range for OpenGL
		unsigned int color = d_8to24table[(int)p->color];
		float b = ((color >> 16) & 0xFF) / 255.0f;
		float g = ((color >> 8) & 0xFF) / 255.0f;
		float r = (color & 0xFF) / 255.0f;

		// Calculate remaining life as a percentage
		float lifePercentage = 0.7f - ((float)(cl.time - p->start) / (float)(p->die - p->start));

		// Ensure it doesn't exceed 1.0 or go below 0.0
		lifePercentage =(lifePercentage > 1.0f) ? 1.0f : ((lifePercentage < 0.0f) ? 0.0f : lifePercentage);

		// Fade out alpha based on remaining life
		float alpha = lifePercentage;

		// Set the color with alpha
		glColor4f(r, g, b, alpha);

		glTexCoord2f (0,0);
		glVertex3fv (p->org);
		glTexCoord2f (1,0);
		glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
		glTexCoord2f (0,1);
		glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);
#else
		D_DrawParticle (p);
#endif
		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >= 8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >= 8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i = 0; i < 3; i++)
				p->vel[i] -= p->vel[i] * frametime;
			p->vel[2] -= grav;
			break;

		case pt_explode3: // New case with unique behavior
			p->ramp += (time2 + time3) / 2; // Average of time2 and time3 for ramp increase
			if (p->ramp >= 6) // Slightly shorter life span
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp % 5]; // Assume ramp3 is a new color array, cycling through 5 colors
			for (i = 0; i < 2; i++) // Only modify x and y velocity
				p->vel[i] *= 1.1; // Slightly increase horizontal velocity
			p->vel[2] -= grav * 0.5; // Reduced gravitational effect
			break;

		case pt_smoke:
			p->ramp += time1; // Gradual increase to simulate smoke dispersal
			if (p->ramp >= 10) // Allowing for a longer lifespan
				p->die = -1;
			//else
			//	p->color = ramp1[(int)p->ramp % 6]; // Assuming ramp4 has smoke-like colors, cycling through 6 shades

			// Smoke particles move slower than explosion particles, simulate this
			for (i = 0; i < 3; i++)
				p->vel[i] *= 0.8; // Apply a damping effect to slow down over time

			// Smoke rises, so we adjust the vertical velocity differently
			p->vel[2] += 10 - (grav * 0.2); // Smoke rises initially then slowly settles under gravity

			break;

		case pt_blob:
			for (i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i = 0; i < 2; i++)
				p->vel[i] -= p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;

		case pt_firetrail:
			p->color = ramp3[(int)p->ramp];
		case pt_smoketrail:
		case pt_blood: // Handling blood particle behavior
			{
				p->ramp += time1; // Use a different time variable for gradual changes
				if (p->ramp >= 5) // Shorter lifespan for blood particles
					p->die = -1;

				for (i = 0; i < 3; i++) {
					float randomFactor = 0.8 + ((rand() % 401) / 1000.0); // Generates a random value between 0.8 and 1.2
					p->vel[i] *= randomFactor; // Apply a random damping effect to slow down or slightly speed up over time
				}
			}
			break;

		case pt_grav:
#ifdef QUAKE2
			p->vel[2] -= grav * 20;
			break;
#endif
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;
		}
	}

#ifdef GLQUAKE
	glEnd ();
	glDisable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#else
	D_EndParticles ();
#endif
}

