#include "test.hpp"

namespace
{
	char const* VERT_SHADER_SOURCE("gl-500/buffer-pinned-amd.vert");
	char const* FRAG_SHADER_SOURCE("gl-500/buffer-pinned-amd.frag");
	char const* TEXTURE_DIFFUSE("kueken7_rgba8_srgb.dds");

	GLsizei const VertexCount(4);
	GLsizeiptr const VertexSize = VertexCount * sizeof(glf::vertex_v2fv2f);
	glf::vertex_v2fv2f const VertexData[VertexCount] =
	{
		glf::vertex_v2fv2f(glm::vec2(-1.0f,-1.0f), glm::vec2(0.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f,-1.0f), glm::vec2(1.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
		glf::vertex_v2fv2f(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 0.0f))
	};

	GLsizei const ElementCount(6);
	GLsizeiptr const ElementSize = ElementCount * sizeof(GLushort);
	GLushort const ElementData[ElementCount] =
	{
		0, 1, 2, 
		2, 3, 0
	};

	namespace program
	{
		enum type
		{
			VERTEX,
			FRAGMENT,
			MAX
		};
	}//namespace program

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

	std::size_t const PAGE_SIZE = 4096;
}//namespace

class gl_500_buffer_pinned_amd : public framework
{
public:
	gl_500_buffer_pinned_amd(int argc, char* argv[]) :
		framework(argc, argv, "gl-500-buffer-pinned-amd", framework::CORE, 4, 2),
		PipelineName(0),
		ProgramName(0),
		VertexArrayName(0),
		TextureName(0)
	{}

private:
	std::unique_ptr<glm::byte> ClientBufferAddress;
	std::array<GLuint, buffer::MAX> BufferName;
	GLuint PipelineName;
	GLuint ProgramName;
	GLuint VertexArrayName;
	GLuint TextureName;

	bool initProgram()
	{
		bool Validated(true);
	
		if(Validated)
		{
			compiler Compiler;
			GLuint VertShaderName = Compiler.create(GL_VERTEX_SHADER, getDataDirectory() + VERT_SHADER_SOURCE, "--version 420 --profile core");
			GLuint FragShaderName = Compiler.create(GL_FRAGMENT_SHADER, getDataDirectory() + FRAG_SHADER_SOURCE, "--version 420 --profile core");
			Validated = Validated && Compiler.check();

			ProgramName = glCreateProgram();
			glProgramParameteri(ProgramName, GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName, VertShaderName);
			glAttachShader(ProgramName, FragShaderName);
			glLinkProgram(ProgramName);

			Validated = Validated && Compiler.check_program(ProgramName);
		}

		if(Validated)
		{
			glGenProgramPipelines(1, &PipelineName);
			glUseProgramStages(PipelineName, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, ProgramName);
		}

		return Validated;
	}

	glm::byte* align(glm::byte* Address, std::size_t PageSize)
	{
		std::uintptr_t Tmp = reinterpret_cast<std::uintptr_t>(Address - 1);
		return reinterpret_cast<glm::byte*>(Tmp + (PageSize - (Tmp % PageSize)));
	}

	void* initMemoryBuffer()
	{
		this->ClientBufferAddress.reset(new glm::byte[VertexSize + PAGE_SIZE - 1]);
		glm::byte* UnalignAddress = this->ClientBufferAddress.get();
		glm::byte* AlignAddress = align(UnalignAddress, PAGE_SIZE);
		memcpy(AlignAddress, VertexData, VertexSize);
		return reinterpret_cast<void*>(AlignAddress);
	}

	bool initBuffer()
	{
		bool Validated(true);

		glGenBuffers(buffer::MAX, &BufferName[0]);

		GLint UniformBufferOffset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffset);
		GLint UniformBlockSize = glm::max(GLint(sizeof(glm::mat4)), UniformBufferOffset);

		glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
		glBufferData(GL_UNIFORM_BUFFER, UniformBlockSize, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementSize, ElementData, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		void* Pointer = initMemoryBuffer();

		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, BufferName[buffer::VERTEX]);
		glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, VertexSize, Pointer, GL_STREAM_COPY);
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0);
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0);

		return Validated;
	}

	bool initTexture()
	{
		bool Validated(true);

		gli::texture2d Texture(gli::load_dds((getDataDirectory() + TEXTURE_DIFFUSE).c_str()));

		gli::gl GL(gli::gl::PROFILE_GL33);
		gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &TextureName);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, TextureName);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(Texture.levels()),
			Format.Internal,
			static_cast<GLsizei>(Texture[0].extent().x), static_cast<GLsizei>(Texture[0].extent().y), static_cast<GLsizei>(1));

		for(gli::texture2d::size_type Level = 0; Level < Texture.levels(); ++Level)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(Level),
				0, 0, 0,
				static_cast<GLsizei>(Texture[Level].extent().x), static_cast<GLsizei>(Texture[Level].extent().y), GLsizei(1),
				Format.External, Format.Type,
				Texture[Level].data());
		}
	
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		return Validated;
	}

	bool initVertexArray()
	{
		bool Validated(true);

		glGenVertexArrays(1, &VertexArrayName);
		glBindVertexArray(VertexArrayName);
			glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
			glVertexAttribPointer(semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(0));
			glVertexAttribPointer(semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(sizeof(glm::vec2)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glEnableVertexAttribArray(semantic::attr::POSITION);
			glEnableVertexAttribArray(semantic::attr::TEXCOORD);
		glBindVertexArray(0);

		return Validated;
	}

	bool begin()
	{
		bool Validated(true);
		Validated = Validated && this->checkExtension("GL_AMD_pinned_memory");

		if(Validated)
			Validated = initTexture();
		if(Validated)
			Validated = initProgram();
		if(Validated)
			Validated = initBuffer();
		if(Validated)
			Validated = initVertexArray();

		return Validated;
	}

	bool end()
	{
		glDeleteProgramPipelines(1, &PipelineName);
		glDeleteProgram(ProgramName);
		glDeleteBuffers(buffer::MAX, &BufferName[0]);
		glDeleteTextures(1, &TextureName);
		glDeleteVertexArrays(1, &VertexArrayName);

		return true;
	}

	bool render()
	{
		glm::vec2 WindowSize(this->getWindowSize());

		{
			glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
			glm::mat4* Pointer = (glm::mat4*)glMapBufferRange(
				GL_UNIFORM_BUFFER, 0,	sizeof(glm::mat4),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

			glm::mat4 Projection = glm::perspective(glm::pi<float>() * 0.25f, WindowSize.x / WindowSize.y, 0.1f, 100.0f);
			glm::mat4 Model = glm::mat4(1.0f);
		
			*Pointer = Projection * this->view() * Model;
		}

		glViewportIndexedf(0, 0, 0, WindowSize.x, WindowSize.y);
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)[0]);

		// Bind rendering objects
		glBindProgramPipeline(PipelineName);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, TextureName);
		glBindBufferBase(GL_UNIFORM_BUFFER, semantic::uniform::TRANSFORM0, BufferName[buffer::TRANSFORM]);
		glBindVertexArray(VertexArrayName);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);

		// Make sure the uniform buffer is uploaded
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, ElementCount, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		return true;
	}
};

int main(int argc, char* argv[])
{
	int Error(0);

	gl_500_buffer_pinned_amd Test(argc, argv);
	Error += Test();

	return Error;
}

