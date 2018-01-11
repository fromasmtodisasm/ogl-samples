#include "test.hpp"

namespace
{
	char const* VERT_SHADER_SOURCE_TEXTURE("gl-450/texture-barrier-blend.vert");
	char const* FRAG_SHADER_SOURCE_TEXTURE("gl-450/texture-barrier-blend.frag");
	char const* VERT_SHADER_SOURCE_SPLASH("gl-450/texture-barrier-splash.vert");
	char const* FRAG_SHADER_SOURCE_SPLASH("gl-450/texture-barrier-splash.frag");
	char const* TEXTURE_DIFFUSE("kueken7_rgb_dxt1_unorm.dds");

	GLsizei const VertexCount(4);
	GLsizeiptr const VertexSize = VertexCount * sizeof(glf::vertex_v2fv2f);
	glf::vertex_v2fv2f const VertexData[VertexCount] =
	{
		glf::vertex_v2fv2f(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
		glf::vertex_v2fv2f(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 0.0f))
	};

	GLsizei const ElementCount(6);
	GLsizeiptr const ElementSize = ElementCount * sizeof(GLushort);
	GLushort const ElementData[ElementCount] =
	{
		0, 1, 2,
		2, 3, 0
	};

	namespace buffer
	{
		enum type
		{
			VERTEX,
			ELEMENT,
			TRANSFORM,
			MAX
		};
	}//namespace buffer

	namespace texture
	{
		enum type
		{
			DIFFUSE,
			COLORBUFFER,
			RENDERBUFFER,
			MAX
		};
	}//namespace texture

	namespace pipeline
	{
		enum type
		{
			TEXTURE,
			SPLASH,
			MAX
		};
	}//namespace pipeline

	GLuint FramebufferName(0);
	std::vector<GLuint> ProgramName(pipeline::MAX);
	std::vector<GLuint> VertexArrayName(pipeline::MAX);
	std::vector<GLuint> BufferName(buffer::MAX);
	std::vector<GLuint> TextureName(texture::MAX);
	std::vector<GLuint> PipelineName(pipeline::MAX);
}//namespace

class gl_450_texture_barrier : public framework
{
public:
	gl_450_texture_barrier(int argc, char* argv[]) :
		framework(argc, argv, "gl-450-texture-barrier", framework::CORE, 4, 2, glm::vec2(0.0f))
	{}

private:
	std::vector<glm::vec4> Viewports;

	bool initProgram()
	{
		bool Validated(true);

		if (Validated)
		{
			compiler Compiler;
			GLuint VertShaderName = Compiler.create(GL_VERTEX_SHADER, getDataDirectory() + VERT_SHADER_SOURCE_TEXTURE, "--version 420 --profile core");
			GLuint FragShaderName = Compiler.create(GL_FRAGMENT_SHADER, getDataDirectory() + FRAG_SHADER_SOURCE_TEXTURE, "--version 420 --profile core");
			Validated = Validated && Compiler.check();

			ProgramName[pipeline::TEXTURE] = glCreateProgram();
			glProgramParameteri(ProgramName[pipeline::TEXTURE], GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName[pipeline::TEXTURE], VertShaderName);
			glAttachShader(ProgramName[pipeline::TEXTURE], FragShaderName);
			glLinkProgram(ProgramName[pipeline::TEXTURE]);

			Validated = Validated && Compiler.check_program(ProgramName[pipeline::TEXTURE]);
		}

		if (Validated)
		{
			compiler Compiler;
			GLuint VertShaderName = Compiler.create(GL_VERTEX_SHADER, getDataDirectory() + VERT_SHADER_SOURCE_SPLASH, "--version 420 --profile core");
			GLuint FragShaderName = Compiler.create(GL_FRAGMENT_SHADER, getDataDirectory() + FRAG_SHADER_SOURCE_SPLASH, "--version 420 --profile core");
			Validated = Validated && Compiler.check();

			ProgramName[pipeline::SPLASH] = glCreateProgram();
			glProgramParameteri(ProgramName[pipeline::SPLASH], GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName[pipeline::SPLASH], VertShaderName);
			glAttachShader(ProgramName[pipeline::SPLASH], FragShaderName);
			glLinkProgram(ProgramName[pipeline::SPLASH]);

			Validated = Validated && Compiler.check_program(ProgramName[pipeline::SPLASH]);
		}

		if (Validated)
		{
			glGenProgramPipelines(pipeline::MAX, &PipelineName[0]);
			glUseProgramStages(PipelineName[pipeline::SPLASH], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, ProgramName[pipeline::SPLASH]);
			glUseProgramStages(PipelineName[pipeline::TEXTURE], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, ProgramName[pipeline::TEXTURE]);
		}

		return Validated;
	}

	bool initBuffer()
	{
		glGenBuffers(buffer::MAX, &BufferName[0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementSize, ElementData, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
		glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		GLint UniformBufferOffset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffset);
		GLint UniformBlockSize = glm::max(GLint(sizeof(glm::mat4)), UniformBufferOffset);

		glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
		glBufferData(GL_UNIFORM_BUFFER, UniformBlockSize, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return true;
	}

	bool initTexture()
	{
		bool Validated(true);

		gli::gl GL(gli::gl::PROFILE_GL33);
		gli::texture2d Texture(gli::load_dds((getDataDirectory() + TEXTURE_DIFFUSE).c_str()));
		gli::gl::format const& Format = GL.translate(Texture.format(), Texture.swizzles());
		assert(!Texture.empty());

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(texture::MAX, &TextureName[0]);

		glBindTexture(GL_TEXTURE_2D, TextureName[texture::DIFFUSE]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, GLint(Texture.levels() - 1));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);
		glTexStorage2D(GL_TEXTURE_2D, static_cast<GLint>(Texture.levels()), Format.Internal, static_cast<GLsizei>(Texture.extent().x), static_cast<GLsizei>(Texture.extent().y));

		for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
		{
			glCompressedTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(Level),
				0, 0,
				static_cast<GLsizei>(Texture[Level].extent().x), static_cast<GLsizei>(Texture[Level].extent().y),
				Format.Internal,
				static_cast<GLsizei>(Texture[Level].size()),
				Texture[Level].data());
		}

		glm::vec2 WindowSize(this->getWindowSize());

		glBindTexture(GL_TEXTURE_2D, TextureName[texture::COLORBUFFER]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, static_cast<GLint>(1), GL_RGBA8, static_cast<GLsizei>(WindowSize.x), static_cast<GLsizei>(WindowSize.y));

		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		return Validated;
	}

	bool initVertexArray()
	{
		glGenVertexArrays(pipeline::MAX, &VertexArrayName[0]);
		glBindVertexArray(VertexArrayName[pipeline::TEXTURE]);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
		glVertexAttribPointer(semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(0));
		glVertexAttribPointer(semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(sizeof(glm::vec2)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(semantic::attr::POSITION);
		glEnableVertexAttribArray(semantic::attr::TEXCOORD);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);
		glBindVertexArray(0);

		glBindVertexArray(VertexArrayName[pipeline::SPLASH]);
		glBindVertexArray(0);

		return true;
	}

	bool initFramebuffer()
	{
		bool Validated(true);

		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, TextureName[texture::COLORBUFFER], 0);

		if(!this->checkFramebuffer(FramebufferName))
			return false;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return true;
	}

	bool begin()
	{
		bool Validated = this->checkExtension("GL_ARB_texture_barrier");

		glm::vec2 const WindowSize(this->getWindowSize());
		glm::vec2 const WindowRange = WindowSize * 3.f;

		this->Viewports.resize(1000);
		for (std::size_t i = 0; i < this->Viewports.size(); ++i)
		{
			glm::vec2 const ViewportPos(i % 17u, i % 13u);
			glm::vec2 const ViewportSize(i % 11u);
			this->Viewports[i] = glm::vec4(ViewportPos / glm::vec2(17, 13) * WindowRange - WindowSize, WindowSize * ViewportSize / glm::vec2(11));
		}
/*
		glm::vec2 WindowSize(this->getWindowSize());
		this->Viewports.resize(1000);
		for (std::size_t i = 0; i < this->Viewports.size(); ++i)
		{
			this->Viewports[i] = glm::vec4(
				glm::linearRand(-WindowSize.x, WindowSize.x * 2.0f), 
				glm::linearRand(-WindowSize.y, WindowSize.y * 2.0f),
				WindowSize * glm::linearRand(0.0f, 1.0f));
		}
*/
		caps Caps(caps::CORE);

		if (Validated)
			Validated = initProgram();
		if (Validated)
			Validated = initBuffer();
		if (Validated)
			Validated = initVertexArray();
		if (Validated)
			Validated = initTexture();
		if (Validated)
			Validated = initFramebuffer();

		return Validated;
	}

	bool end()
	{
		glDeleteProgramPipelines(pipeline::MAX, &PipelineName[0]);
		glDeleteProgram(ProgramName[pipeline::SPLASH]);
		glDeleteProgram(ProgramName[pipeline::TEXTURE]);
		glDeleteBuffers(buffer::MAX, &BufferName[0]);
		glDeleteFramebuffers(1, &FramebufferName);
		glDeleteTextures(texture::MAX, &TextureName[0]);
		glDeleteVertexArrays(pipeline::MAX, &VertexArrayName[0]);

		return true;
	}

	bool render()
	{
		glm::vec2 WindowSize(this->getWindowSize());

		{
			glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
			glm::mat4* Pointer = (glm::mat4*)glMapBufferRange(
				GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

			glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, WindowSize.x / WindowSize.y, 0.1f, 100.0f);
			glm::mat4 Model = glm::mat4(1.0f);

			*Pointer = Projection * this->view() * Model;

			// Make sure the uniform buffer is uploaded
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		glClearTexImage(TextureName[texture::COLORBUFFER], 0, GL_RGBA, GL_FLOAT, &glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)[0]);

		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

		// Bind rendering objects
		glBindProgramPipeline(PipelineName[pipeline::TEXTURE]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureName[texture::DIFFUSE]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, TextureName[texture::COLORBUFFER]);
		glBindVertexArray(VertexArrayName[pipeline::TEXTURE]);
		glBindBufferBase(GL_UNIFORM_BUFFER, semantic::uniform::TRANSFORM0, BufferName[buffer::TRANSFORM]);

		for (std::size_t i = 0; i < this->Viewports.size(); ++i)
		{
			glViewportIndexedfv(0, &this->Viewports[i][0]);
			glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, ElementCount, GL_UNSIGNED_SHORT, 0, 1, 0, 0);
			glTextureBarrier();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewportIndexedf(0, 0, 0, WindowSize.x, WindowSize.y);

		glBindProgramPipeline(PipelineName[pipeline::SPLASH]);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VertexArrayName[pipeline::SPLASH]);
		glBindTexture(GL_TEXTURE_2D, TextureName[texture::COLORBUFFER]);

		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);

		return true;
	}
};

int main(int argc, char* argv[])
{
	int Error(0);

	gl_450_texture_barrier Test(argc, argv);
	Error += Test();

	return Error;
}

