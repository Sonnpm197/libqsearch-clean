import React, {useEffect, useState} from "react";
import axios from 'axios';
import {BACKEND_BASE_URL} from '../config/api.js'
import {getLoginUser} from '../functions/user.js';

const Header = ({openLogin, setOpenLogin, setAuthenticated}) => {

    const [userName, setUserName] = useState(null);

    const openModal = () => {
        setOpenLogin(true);
    };

    const closeModal = () => {
        setOpenLogin(false);
    };

    const handleGoogleLogin = async () => {
        try {
            window.location.href = `${BACKEND_BASE_URL}/auth/google`;
        } catch (error) {
            console.error('Google login failed', error);
        }
    };

    const handleGithubLogin = async () => {
        try {
            window.location.href = `${BACKEND_BASE_URL}/auth/github`;
        } catch (error) {
            console.error('Github login failed', error);
        }
    };

    const fetchUserData = async () => {
        const authUserName = await getLoginUser();
        setUserName(authUserName);
        setAuthenticated(!!authUserName);
        console.log(`User is authenticated ${!!authUserName} ${authUserName}`);
    };

    useEffect(() => {
        fetchUserData();
    }, []);

    const handleLogout = async () => {
        try {
            const res = await axios.get(`${BACKEND_BASE_URL}/auth/logout`, { withCredentials: true });
            console.log('handleLogout response', res);
            const returnedUser = res.data.user;
            setUserName(returnedUser);
            setAuthenticated(!!returnedUser);
            console.log(`User is authenticated ${!!returnedUser}`);
        } catch (error) {
            console.log(error);
        }
    };

    return (
        <header className="flex justify-between items-center px-10 py-4 text-white">
            <div className="text-4xl font-bold">NCD Calculator</div>
            {!userName ? (
                <button
                    className="text-white bg-blue-500 px-4 py-2 rounded-lg focus:outline-none"
                    onClick={openModal}>
                    Login
                </button>
            ) : (
                <div className="flex items-center space-x-4">
                    <span className="text-white">{userName}</span>
                    <button
                        className="bg-red-500 text-white px-4 py-2 rounded-full hover:bg-red-600"
                        onClick={handleLogout}>
                        Logout
                    </button>
                </div>
            )}

            {openLogin && (
                <div
                    className="fixed z-50 inset-0 flex justify-center items-center bg-black bg-opacity-50 backdrop-blur-sm"
                    onClick={closeModal}>
                    <div
                        className="bg-white p-6 rounded-lg w-80" onClick={(e) => e.stopPropagation()}>
                        <h2 className="text-center text-xl font-semibold mb-4 text-black">Login to continue</h2>
                        <div className="flex flex-col items-center space-y-4">
                            <button
                                className="bg-red-500 text-white px-6 py-2 rounded-full w-full"
                                onClick={handleGoogleLogin}>
                                Login with Google
                            </button>
                            <button
                                className="bg-green-700 text-white px-6 py-2 rounded-full w-full"
                                onClick={handleGithubLogin}>
                                Login with Github
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </header>
    );
};

export default Header;