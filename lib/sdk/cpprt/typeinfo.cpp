/*
 * PROJECT:         ReactOS c++ runtime library
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Type info stub implementation
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

/* TODO: #include <exception> instead */
class type_info {
public:
    __declspec(dllimport) virtual ~type_info();
private:
    type_info(const type_info &);
    type_info &operator=(const type_info &);
};

/* These stubs don't need to do anything (those private functions are never
 * called). They need to be in cpprt, though, in order to have the vtable
 * and generated destructor thunks available to programs */
type_info::type_info(const type_info &)
{
}

type_info &type_info::operator=(const type_info &)
{
    return *this;
}
